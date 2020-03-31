

#include <ripple/rpc/impl/TransactionSign.h>
#include <ripple/app/ledger/LedgerMaster.h>
#include <ripple/app/ledger/OpenLedger.h>
#include <ripple/app/main/Application.h>
#include <ripple/app/misc/LoadFeeTrack.h>
#include <ripple/app/misc/Transaction.h>
#include <ripple/app/misc/TxQ.h>
#include <ripple/app/paths/Pathfinder.h>
#include <ripple/app/tx/apply.h>              
#include <ripple/basics/Log.h>
#include <ripple/basics/mulDiv.h>
#include <ripple/json/json_writer.h>
#include <ripple/net/RPCErr.h>
#include <ripple/protocol/Sign.h>
#include <ripple/protocol/ErrorCodes.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/STAccount.h>
#include <ripple/protocol/STParsedJSON.h>
#include <ripple/protocol/TxFlags.h>
#include <ripple/rpc/impl/LegacyPathFind.h>
#include <ripple/rpc/impl/RPCHelpers.h>
#include <ripple/rpc/impl/Tuning.h>
#include <algorithm>
#include <iterator>

namespace ripple {
namespace RPC {
namespace detail {

class SigningForParams
{
private:
    AccountID const* const multiSigningAcctID_;
    PublicKey* const multiSignPublicKey_;
    Buffer* const multiSignature_;

public:
    explicit SigningForParams ()
    : multiSigningAcctID_ (nullptr)
    , multiSignPublicKey_ (nullptr)
    , multiSignature_ (nullptr)
    { }

    SigningForParams (SigningForParams const& rhs) = delete;

    SigningForParams (
        AccountID const& multiSigningAcctID,
        PublicKey& multiSignPublicKey,
        Buffer& multiSignature)
    : multiSigningAcctID_ (&multiSigningAcctID)
    , multiSignPublicKey_ (&multiSignPublicKey)
    , multiSignature_ (&multiSignature)
    { }

    bool isMultiSigning () const
    {
        return ((multiSigningAcctID_ != nullptr) &&
                (multiSignPublicKey_ != nullptr) &&
                (multiSignature_ != nullptr));
    }

    bool isSingleSigning () const
    {
        return !isMultiSigning();
    }

    bool editFields () const
    {
        return !isMultiSigning();
    }

    AccountID const& getSigner ()
    {
        return *multiSigningAcctID_;
    }

    void setPublicKey (PublicKey const& multiSignPublicKey)
    {
        *multiSignPublicKey_ = multiSignPublicKey;
    }

    void moveMultiSignature (Buffer&& multiSignature)
    {
        *multiSignature_ = std::move (multiSignature);
    }
};


static error_code_i acctMatchesPubKey (
    std::shared_ptr<SLE const> accountState,
    AccountID const& accountID,
    PublicKey const& publicKey)
{
    auto const publicKeyAcctID = calcAccountID(publicKey);
    bool const isMasterKey = publicKeyAcctID == accountID;

    if (!accountState)
    {
        if (isMasterKey)
            return rpcSUCCESS;
        return rpcBAD_SECRET;
    }

    auto const& sle = *accountState;
    if (isMasterKey)
    {
        if (sle.isFlag(lsfDisableMaster))
            return rpcMASTER_DISABLED;
        return rpcSUCCESS;
    }

    if ((sle.isFieldPresent (sfRegularKey)) &&
        (publicKeyAcctID == sle.getAccountID (sfRegularKey)))
    {
        return rpcSUCCESS;
    }
    return rpcBAD_SECRET;
}

static Json::Value checkPayment(
    Json::Value const& params,
    Json::Value& tx_json,
    AccountID const& srcAddressID,
    Role const role,
    Application& app,
    std::shared_ptr<ReadView const> const& ledger,
    bool doPath)
{
    if (tx_json[jss::TransactionType].asString () != jss::Payment)
        return Json::Value();

    if (!tx_json.isMember (jss::Amount))
        return RPC::missing_field_error ("tx_json.Amount");

    STAmount amount;

    if (! amountFromJsonNoThrow (amount, tx_json [jss::Amount]))
        return RPC::invalid_field_error ("tx_json.Amount");

    if (!tx_json.isMember (jss::Destination))
        return RPC::missing_field_error ("tx_json.Destination");

    auto const dstAccountID = parseBase58<AccountID>(
        tx_json[jss::Destination].asString());
    if (! dstAccountID)
        return RPC::invalid_field_error ("tx_json.Destination");

    if ((doPath == false) && params.isMember (jss::build_path))
        return RPC::make_error (rpcINVALID_PARAMS,
            "Field 'build_path' not allowed in this context.");

    if (tx_json.isMember (jss::Paths) && params.isMember (jss::build_path))
        return RPC::make_error (rpcINVALID_PARAMS,
            "Cannot specify both 'tx_json.Paths' and 'build_path'");

    if (!tx_json.isMember (jss::Paths) && params.isMember (jss::build_path))
    {
        STAmount    sendMax;

        if (tx_json.isMember (jss::SendMax))
        {
            if (! amountFromJsonNoThrow (sendMax, tx_json [jss::SendMax]))
                return RPC::invalid_field_error ("tx_json.SendMax");
        }
        else
        {
            sendMax = amount;
            sendMax.setIssuer (srcAddressID);
        }

        if (sendMax.native () && amount.native ())
            return RPC::make_error (rpcINVALID_PARAMS,
                "Cannot build XRP to XRP paths.");

        {
            LegacyPathFind lpf (isUnlimited (role), app);
            if (!lpf.isOk ())
                return rpcError (rpcTOO_BUSY);

            STPathSet result;
            if (ledger)
            {
                Pathfinder pf(std::make_shared<RippleLineCache>(ledger),
                    srcAddressID, *dstAccountID, sendMax.issue().currency,
                        sendMax.issue().account, amount, boost::none, app);
                if (pf.findPaths(app.config().PATH_SEARCH_OLD))
                {
                    pf.computePathRanks(4);
                    STPath fullLiquidityPath;
                    STPathSet paths;
                    result = pf.getBestPaths(4, fullLiquidityPath, paths,
                        sendMax.issue().account);
                }
            }

            auto j = app.journal ("RPCHandler");
            JLOG (j.debug())
                << "transactionSign: build_path: "
                << result.getJson (JsonOptions::none);

            if (! result.empty ())
                tx_json[jss::Paths] = result.getJson (JsonOptions::none);
        }
    }
    return Json::Value();
}


static std::pair<Json::Value, AccountID>
checkTxJsonFields (
    Json::Value const& tx_json,
    Role const role,
    bool const verify,
    std::chrono::seconds validatedLedgerAge,
    Config const& config,
    LoadFeeTrack const& feeTrack)
{
    std::pair<Json::Value, AccountID> ret;

    if (!tx_json.isObject())
    {
        ret.first = RPC::object_field_error (jss::tx_json);
        return ret;
    }

    if (! tx_json.isMember (jss::TransactionType))
    {
        ret.first = RPC::missing_field_error ("tx_json.TransactionType");
        return ret;
    }

    if (! tx_json.isMember (jss::Account))
    {
        ret.first = RPC::make_error (rpcSRC_ACT_MISSING,
            RPC::missing_field_message ("tx_json.Account"));
        return ret;
    }

    auto const srcAddressID = parseBase58<AccountID>(
        tx_json[jss::Account].asString());

    if (! srcAddressID)
    {
        ret.first = RPC::make_error (rpcSRC_ACT_MALFORMED,
            RPC::invalid_field_message ("tx_json.Account"));
        return ret;
    }

    if (verify && !config.standalone() &&
        (validatedLedgerAge > Tuning::maxValidatedLedgerAge))
    {
        ret.first = rpcError (rpcNO_CURRENT);
        return ret;
    }

    if (feeTrack.isLoadedCluster() && !isUnlimited (role))
    {
        ret.first = rpcError (rpcTOO_BUSY);
        return ret;
    }

    ret.second = *srcAddressID;
    return ret;
}


struct transactionPreProcessResult
{
    Json::Value const first;
    std::shared_ptr<STTx> const second;

    transactionPreProcessResult () = delete;
    transactionPreProcessResult (transactionPreProcessResult const&) = delete;
    transactionPreProcessResult (transactionPreProcessResult&& rhs)
    : first (std::move (rhs.first))    
    , second (std::move (rhs.second))
    { }

    transactionPreProcessResult& operator=
        (transactionPreProcessResult const&) = delete;
     transactionPreProcessResult& operator=
        (transactionPreProcessResult&&) = delete;

    transactionPreProcessResult (Json::Value&& json)
    : first (std::move (json))
    , second ()
    { }

    explicit transactionPreProcessResult (std::shared_ptr<STTx>&& st)
    : first ()
    , second (std::move (st))
    { }
};

static
transactionPreProcessResult
transactionPreProcessImpl (
    Json::Value& params,
    Role role,
    SigningForParams& signingArgs,
    std::chrono::seconds validatedLedgerAge,
    Application& app,
    std::shared_ptr<OpenView const> const& ledger)
{
    auto j = app.journal ("RPCHandler");

    Json::Value jvResult;
    auto const keypair = keypairForSignature (params, jvResult);
    if (contains_error (jvResult))
        return std::move (jvResult);

    bool const verify = !(params.isMember (jss::offline)
                          && params[jss::offline].asBool());

    if (! params.isMember (jss::tx_json))
        return RPC::missing_field_error (jss::tx_json);

    Json::Value& tx_json (params [jss::tx_json]);

    auto txJsonResult = checkTxJsonFields (
        tx_json, role, verify, validatedLedgerAge,
        app.config(), app.getFeeTrack());

    if (RPC::contains_error (txJsonResult.first))
        return std::move (txJsonResult.first);

    auto const srcAddressID = txJsonResult.second;

    if (!verify && !tx_json.isMember (jss::Sequence))
        return RPC::missing_field_error ("tx_json.Sequence");

    std::shared_ptr<SLE const> sle = ledger->read(
            keylet::account(srcAddressID));

    if (verify && !sle)
    {
        JLOG (j.debug())
            << "transactionSign: Failed to find source account "
            << "in current ledger: "
            << toBase58(srcAddressID);

        return rpcError (rpcSRC_ACT_NOT_FOUND);
    }

    {
        Json::Value err = checkFee (
            params,
            role,
            verify && signingArgs.editFields(),
            app.config(),
            app.getFeeTrack(),
            app.getTxQ(),
            ledger);

        if (RPC::contains_error (err))
            return std::move (err);

        err = checkPayment (
            params,
            tx_json,
            srcAddressID,
            role,
            app,
            ledger,
            verify && signingArgs.editFields());

        if (RPC::contains_error(err))
            return std::move (err);
    }

    if (signingArgs.editFields())
    {
        if (!tx_json.isMember (jss::Sequence))
        {
            if (! sle)
            {
                JLOG (j.debug())
                << "transactionSign: Failed to find source account "
                << "in current ledger: "
                << toBase58(srcAddressID);

                return rpcError (rpcSRC_ACT_NOT_FOUND);
            }

            auto seq = (*sle)[sfSequence];
            auto const queued = app.getTxQ().getAccountTxs(srcAddressID,
                *ledger);
            for(auto const& tx : queued)
            {
                if (tx.first == seq)
                    ++seq;
                else if (tx.first > seq)
                    break;
            }
            tx_json[jss::Sequence] = seq;
        }

        if (!tx_json.isMember (jss::Flags))
            tx_json[jss::Flags] = tfFullyCanonicalSig;
    }

    if (signingArgs.isMultiSigning())
    {
        if (tx_json.isMember (sfTxnSignature.jsonName))
            return rpcError (rpcALREADY_SINGLE_SIG);

        signingArgs.setPublicKey (keypair.first);
    }
    else if (signingArgs.isSingleSigning())
    {
        if (tx_json.isMember (sfSigners.jsonName))
            return rpcError (rpcALREADY_MULTISIG);
    }

    if (verify)
    {
        if (! sle)
            return rpcError (rpcSRC_ACT_NOT_FOUND);

        JLOG (j.trace())
            << "verify: " << toBase58(calcAccountID(keypair.first))
            << " : " << toBase58(srcAddressID);

        if (!signingArgs.isMultiSigning())
        {
            auto const err = acctMatchesPubKey (
                sle, srcAddressID, keypair.first);

            if (err != rpcSUCCESS)
                return rpcError (err);
        }
    }

    STParsedJSONObject parsed (std::string (jss::tx_json), tx_json);
    if (parsed.object == boost::none)
    {
        Json::Value err;
        err [jss::error] = parsed.error [jss::error];
        err [jss::error_code] = parsed.error [jss::error_code];
        err [jss::error_message] = parsed.error [jss::error_message];
        return std::move (err);
    }

    std::shared_ptr<STTx> stpTrans;
    try
    {
        parsed.object->setFieldVL (sfSigningPubKey,
            signingArgs.isMultiSigning()
                ? Slice (nullptr, 0)
                : keypair.first.slice());

        stpTrans = std::make_shared<STTx> (
            std::move (parsed.object.get()));
    }
    catch (STObject::FieldErr& err)
    {
        return RPC::make_error (rpcINVALID_PARAMS, err.what());
    }
    catch (std::exception&)
    {
        return RPC::make_error (rpcINTERNAL,
            "Exception occurred constructing serialized transaction");
    }

    std::string reason;
    if (!passesLocalChecks (*stpTrans, reason))
        return RPC::make_error (rpcINVALID_PARAMS, reason);

    if (signingArgs.isMultiSigning ())
    {
        Serializer s = buildMultiSigningData (*stpTrans,
            signingArgs.getSigner ());

        auto multisig = ripple::sign (
            keypair.first,
            keypair.second,
            s.slice());

        signingArgs.moveMultiSignature (std::move (multisig));
    }
    else if (signingArgs.isSingleSigning())
    {
        stpTrans->sign (keypair.first, keypair.second);
    }

    return transactionPreProcessResult {std::move (stpTrans)};
}

static
std::pair <Json::Value, Transaction::pointer>
transactionConstructImpl (std::shared_ptr<STTx const> const& stpTrans,
    Rules const& rules, Application& app)
{
    std::pair <Json::Value, Transaction::pointer> ret;

    Transaction::pointer tpTrans;
    {
        std::string reason;
        tpTrans = std::make_shared<Transaction>(
            stpTrans, reason, app);
        if (tpTrans->getStatus () != NEW)
        {
            ret.first = RPC::make_error (rpcINTERNAL,
                "Unable to construct transaction: " + reason);
            return ret;
        }
    }
    try
    {
        {
            Serializer s;
            tpTrans->getSTransaction ()->add (s);
            Blob transBlob = s.getData ();
            SerialIter sit {makeSlice(transBlob)};

            auto sttxNew = std::make_shared<STTx const> (sit);
            if (!app.checkSigs())
                forceValidity(app.getHashRouter(),
                    sttxNew->getTransactionID(), Validity::SigGoodOnly);
            if (checkValidity(app.getHashRouter(),
                *sttxNew, rules, app.config()).first != Validity::Valid)
            {
                ret.first = RPC::make_error (rpcINTERNAL,
                    "Invalid signature.");
                return ret;
            }

            std::string reason;
            auto tpTransNew =
                std::make_shared<Transaction> (sttxNew, reason, app);

            if (tpTransNew)
            {
                if (!tpTransNew->getSTransaction()->isEquivalent (
                        *tpTrans->getSTransaction()))
                {
                    tpTransNew.reset ();
                }
                tpTrans = std::move (tpTransNew);
            }
        }
    }
    catch (std::exception&)
    {
        tpTrans.reset ();
    }

    if (!tpTrans)
    {
        ret.first = RPC::make_error (rpcINTERNAL,
            "Unable to sterilize transaction.");
        return ret;
    }
    ret.second = std::move (tpTrans);
    return ret;
}

static Json::Value transactionFormatResultImpl (Transaction::pointer tpTrans)
{
    Json::Value jvResult;
    try
    {
        jvResult[jss::tx_json] = tpTrans->getJson (JsonOptions::none);
        jvResult[jss::tx_blob] = strHex (
            tpTrans->getSTransaction ()->getSerializer ().peekData ());

        if (temUNCERTAIN != tpTrans->getResult ())
        {
            std::string sToken;
            std::string sHuman;

            transResultInfo (tpTrans->getResult (), sToken, sHuman);

            jvResult[jss::engine_result]           = sToken;
            jvResult[jss::engine_result_code]      = tpTrans->getResult ();
            jvResult[jss::engine_result_message]   = sHuman;
        }
    }
    catch (std::exception&)
    {
        jvResult = RPC::make_error (rpcINTERNAL,
            "Exception occurred during JSON handling.");
    }
    return jvResult;
}

} 


Json::Value checkFee (
    Json::Value& request,
    Role const role,
    bool doAutoFill,
    Config const& config,
    LoadFeeTrack const& feeTrack,
    TxQ const& txQ,
    std::shared_ptr<OpenView const> const& ledger)
{
    Json::Value& tx (request[jss::tx_json]);
    if (tx.isMember (jss::Fee))
        return Json::Value();

    if (! doAutoFill)
        return RPC::missing_field_error ("tx_json.Fee");

    int mult = Tuning::defaultAutoFillFeeMultiplier;
    int div = Tuning::defaultAutoFillFeeDivisor;
    if (request.isMember (jss::fee_mult_max))
    {
        if (request[jss::fee_mult_max].isInt())
        {
            mult = request[jss::fee_mult_max].asInt();
            if (mult < 0)
                return RPC::make_error(rpcINVALID_PARAMS,
                    RPC::expected_field_message(jss::fee_mult_max,
                        "a positive integer"));
        }
        else
        {
            return RPC::make_error (rpcHIGH_FEE,
                RPC::expected_field_message (jss::fee_mult_max,
                    "a positive integer"));
        }
    }
    if (request.isMember(jss::fee_div_max))
    {
        if (request[jss::fee_div_max].isInt())
        {
            div = request[jss::fee_div_max].asInt();
            if (div <= 0)
                return RPC::make_error(rpcINVALID_PARAMS,
                    RPC::expected_field_message(jss::fee_div_max,
                        "a positive integer"));
        }
        else
        {
            return RPC::make_error(rpcHIGH_FEE,
                RPC::expected_field_message(jss::fee_div_max,
                    "a positive integer"));
        }
    }

    std::uint64_t const feeDefault = config.TRANSACTION_FEE_BASE;

    std::uint64_t const loadFee =
        scaleFeeLoad (feeDefault, feeTrack,
            ledger->fees(), isUnlimited (role));
    std::uint64_t fee = loadFee;
    {
        auto const metrics = txQ.getMetrics(*ledger);
        auto const baseFee = ledger->fees().base;
        auto escalatedFee = mulDiv(
            metrics.openLedgerFeeLevel, baseFee,
                metrics.referenceFeeLevel).second;
        if (mulDiv(escalatedFee, metrics.referenceFeeLevel,
                baseFee).second < metrics.openLedgerFeeLevel)
            ++escalatedFee;
        fee = std::max(fee, escalatedFee);
    }

    auto const limit = [&]()
    {
        auto const drops = mulDiv (feeDefault,
            ledger->fees().base, ledger->fees().units);
        if (!drops.first)
            Throw<std::overflow_error>("mulDiv");
        auto const result = mulDiv (drops.second, mult, div);
        if (!result.first)
            Throw<std::overflow_error>("mulDiv");
        return result.second;
    }();

    if (fee > limit)
    {
        std::stringstream ss;
        ss << "Fee of " << fee
            << " exceeds the requested tx limit of " << limit;
        return RPC::make_error (rpcHIGH_FEE, ss.str());
    }

    tx [jss::Fee] = static_cast<unsigned int>(fee);
    return Json::Value();
}



Json::Value transactionSign (
    Json::Value jvRequest,
    NetworkOPs::FailHard failType,
    Role role,
    std::chrono::seconds validatedLedgerAge,
    Application& app)
{
    using namespace detail;

    auto const& ledger = app.openLedger().current();
    auto j = app.journal ("RPCHandler");
    JLOG (j.debug()) << "transactionSign: " << jvRequest;

    SigningForParams signForParams;
    transactionPreProcessResult preprocResult = transactionPreProcessImpl (
        jvRequest, role, signForParams,
        validatedLedgerAge, app, ledger);

    if (!preprocResult.second)
        return preprocResult.first;

    std::pair <Json::Value, Transaction::pointer> txn =
        transactionConstructImpl (
            preprocResult.second, ledger->rules(), app);

    if (!txn.second)
        return txn.first;

    return transactionFormatResultImpl (txn.second);
}


Json::Value transactionSubmit (
    Json::Value jvRequest,
    NetworkOPs::FailHard failType,
    Role role,
    std::chrono::seconds validatedLedgerAge,
    Application& app,
    ProcessTransactionFn const& processTransaction)
{
    using namespace detail;

    auto const& ledger = app.openLedger().current();
    auto j = app.journal ("RPCHandler");
    JLOG (j.debug()) << "transactionSubmit: " << jvRequest;


    SigningForParams signForParams;
    transactionPreProcessResult preprocResult = transactionPreProcessImpl (
        jvRequest, role, signForParams, validatedLedgerAge, app, ledger);

    if (!preprocResult.second)
        return preprocResult.first;

    std::pair <Json::Value, Transaction::pointer> txn =
        transactionConstructImpl (
            preprocResult.second, ledger->rules(), app);

    if (!txn.second)
        return txn.first;

    try
    {
        processTransaction (
            txn.second, isUnlimited (role), true, failType);
    }
    catch (std::exception&)
    {
        return RPC::make_error (rpcINTERNAL,
            "Exception occurred during transaction submission.");
    }

    return transactionFormatResultImpl (txn.second);
}

namespace detail
{
static Json::Value checkMultiSignFields (Json::Value const& jvRequest)
{
   if (! jvRequest.isMember (jss::tx_json))
        return RPC::missing_field_error (jss::tx_json);

    Json::Value const& tx_json (jvRequest [jss::tx_json]);

    if (!tx_json.isObject())
        return RPC::invalid_field_message (jss::tx_json);

    if (!tx_json.isMember (jss::Sequence))
        return RPC::missing_field_error ("tx_json.Sequence");

    if (!tx_json.isMember (sfSigningPubKey.getJsonName()))
        return RPC::missing_field_error ("tx_json.SigningPubKey");

    if (!tx_json[sfSigningPubKey.getJsonName()].asString().empty())
        return RPC::make_error (rpcINVALID_PARAMS,
            "When multi-signing 'tx_json.SigningPubKey' must be empty.");

    return Json::Value ();
}

static Json::Value sortAndValidateSigners (
    STArray& signers, AccountID const& signingForID)
{
    if (signers.empty ())
        return RPC::make_param_error ("Signers array may not be empty.");

    std::sort (signers.begin(), signers.end(),
        [](STObject const& a, STObject const& b)
    {
        return (a[sfAccount] < b[sfAccount]);
    });

    auto const dupIter = std::adjacent_find (
        signers.begin(), signers.end(),
        [] (STObject const& a, STObject const& b)
        {
            return (a[sfAccount] == b[sfAccount]);
        });

    if (dupIter != signers.end())
    {
        std::ostringstream err;
        err << "Duplicate Signers:Signer:Account entries ("
            << toBase58((*dupIter)[sfAccount])
            << ") are not allowed.";
        return RPC::make_param_error(err.str ());
    }

    if (signers.end() != std::find_if (signers.begin(), signers.end(),
        [&signingForID](STObject const& elem)
        {
            return elem[sfAccount] == signingForID;
        }))
    {
        std::ostringstream err;
        err << "A Signer may not be the transaction's Account ("
            << toBase58(signingForID) << ").";
        return RPC::make_param_error(err.str ());
    }
    return {};
}

} 


Json::Value transactionSignFor (
    Json::Value jvRequest,
    NetworkOPs::FailHard failType,
    Role role,
    std::chrono::seconds validatedLedgerAge,
    Application& app)
{
    auto const& ledger = app.openLedger().current();
    auto j = app.journal ("RPCHandler");
    JLOG (j.debug()) << "transactionSignFor: " << jvRequest;

    const char accountField[] = "account";

    if (! jvRequest.isMember (accountField))
        return RPC::missing_field_error (accountField);

    auto const signerAccountID = parseBase58<AccountID>(
        jvRequest[accountField].asString());
    if (! signerAccountID)
    {
        return RPC::make_error (rpcSRC_ACT_MALFORMED,
            RPC::invalid_field_message (accountField));
    }

   if (! jvRequest.isMember (jss::tx_json))
        return RPC::missing_field_error (jss::tx_json);

    {
        Json::Value& tx_json (jvRequest [jss::tx_json]);

        if (!tx_json.isObject())
            return RPC::object_field_error (jss::tx_json);

        if (!tx_json.isMember (sfSigningPubKey.getJsonName()))
            tx_json[sfSigningPubKey.getJsonName()] = "";
    }

    using namespace detail;
    {
        Json::Value err = checkMultiSignFields (jvRequest);
        if (RPC::contains_error (err))
            return err;
    }

    Buffer multiSignature;
    PublicKey multiSignPubKey;
    SigningForParams signForParams(
        *signerAccountID, multiSignPubKey, multiSignature);

    transactionPreProcessResult preprocResult = transactionPreProcessImpl (
        jvRequest,
        role,
        signForParams,
        validatedLedgerAge,
        app,
        ledger);

    if (!preprocResult.second)
        return preprocResult.first;

    {
        std::shared_ptr<SLE const> account_state = ledger->read(
                keylet::account(*signerAccountID));
        auto const err = acctMatchesPubKey (
            account_state,
            *signerAccountID,
            multiSignPubKey);

        if (err != rpcSUCCESS)
            return rpcError (err);
    }

    auto& sttx = preprocResult.second;
    {
        STObject signer (sfSigner);
        signer[sfAccount] = *signerAccountID;
        signer.setFieldVL (sfTxnSignature, multiSignature);
        signer.setFieldVL (sfSigningPubKey, multiSignPubKey.slice());

        if (!sttx->isFieldPresent (sfSigners))
            sttx->setFieldArray (sfSigners, {});

        auto& signers = sttx->peekFieldArray (sfSigners);
        signers.emplace_back (std::move (signer));

        auto err = sortAndValidateSigners (signers, (*sttx)[sfAccount]);
        if (RPC::contains_error (err))
            return err;
    }

    std::pair <Json::Value, Transaction::pointer> txn =
        transactionConstructImpl (sttx, ledger->rules(), app);

    if (!txn.second)
        return txn.first;

    return transactionFormatResultImpl (txn.second);
}


Json::Value transactionSubmitMultiSigned (
    Json::Value jvRequest,
    NetworkOPs::FailHard failType,
    Role role,
    std::chrono::seconds validatedLedgerAge,
    Application& app,
    ProcessTransactionFn const& processTransaction)
{
    auto const& ledger = app.openLedger().current();
    auto j = app.journal ("RPCHandler");
    JLOG (j.debug())
        << "transactionSubmitMultiSigned: " << jvRequest;

    using namespace detail;
    {
        Json::Value err = checkMultiSignFields (jvRequest);
        if (RPC::contains_error (err))
            return err;
    }

    Json::Value& tx_json (jvRequest ["tx_json"]);

    auto const txJsonResult = checkTxJsonFields (
        tx_json, role, true, validatedLedgerAge,
        app.config(), app.getFeeTrack());

    if (RPC::contains_error (txJsonResult.first))
        return std::move (txJsonResult.first);

    auto const srcAddressID = txJsonResult.second;

    std::shared_ptr<SLE const> sle = ledger->read(
            keylet::account(srcAddressID));

    if (!sle)
    {
        JLOG (j.debug())
            << "transactionSubmitMultiSigned: Failed to find source account "
            << "in current ledger: "
            << toBase58(srcAddressID);

        return rpcError (rpcSRC_ACT_NOT_FOUND);
    }

    {
        Json::Value err = checkFee (
            jvRequest, role, false, app.config(), app.getFeeTrack(),
                app.getTxQ(), ledger);

        if (RPC::contains_error(err))
            return err;

        err = checkPayment (
            jvRequest,
            tx_json,
            srcAddressID,
            role,
            app,
            ledger,
            false);

        if (RPC::contains_error(err))
            return err;
    }

    std::shared_ptr<STTx> stpTrans;
    {
        STParsedJSONObject parsedTx_json ("tx_json", tx_json);
        if (!parsedTx_json.object)
        {
            Json::Value jvResult;
            jvResult ["error"] = parsedTx_json.error ["error"];
            jvResult ["error_code"] = parsedTx_json.error ["error_code"];
            jvResult ["error_message"] = parsedTx_json.error ["error_message"];
            return jvResult;
        }
        try
        {
            stpTrans = std::make_shared<STTx>(
                std::move(parsedTx_json.object.get()));
        }
        catch (STObject::FieldErr& err)
        {
            return RPC::make_error (rpcINVALID_PARAMS, err.what());
        }
        catch (std::exception& ex)
        {
            std::string reason (ex.what ());
            return RPC::make_error (rpcINTERNAL,
                "Exception while serializing transaction: " + reason);
        }
        std::string reason;
        if (!passesLocalChecks (*stpTrans, reason))
            return RPC::make_error (rpcINVALID_PARAMS, reason);
    }

    {
        if (!stpTrans->getFieldVL (sfSigningPubKey).empty ())
        {
            std::ostringstream err;
            err << "Invalid  " << sfSigningPubKey.fieldName
                << " field.  Field must be empty when multi-signing.";
            return RPC::make_error (rpcINVALID_PARAMS, err.str ());
        }

        if (stpTrans->isFieldPresent (sfTxnSignature))
            return rpcError (rpcSIGNING_MALFORMED);

        auto const fee = stpTrans->getFieldAmount (sfFee);

        if (!isLegalNet (fee))
        {
            std::ostringstream err;
            err << "Invalid " << sfFee.fieldName
                << " field.  Fees must be specified in XRP.";
            return RPC::make_error (rpcINVALID_PARAMS, err.str ());
        }
        if (fee <= 0)
        {
            std::ostringstream err;
            err << "Invalid " << sfFee.fieldName
                << " field.  Fees must be greater than zero.";
            return RPC::make_error (rpcINVALID_PARAMS, err.str ());
        }
    }

    if (! stpTrans->isFieldPresent (sfSigners))
        return RPC::missing_field_error ("tx_json.Signers");

    auto& signers = stpTrans->peekFieldArray (sfSigners);

    if (signers.empty ())
        return RPC::make_param_error("tx_json.Signers array may not be empty.");

    if (std::find_if_not(signers.begin(), signers.end(), [](STObject const& obj)
        {
            return (
                obj.isFieldPresent (sfAccount) &&
                obj.isFieldPresent (sfSigningPubKey) &&
                obj.isFieldPresent (sfTxnSignature) &&
                obj.getCount() == 3);
        }) != signers.end())
    {
        return RPC::make_param_error (
            "Signers array may only contain Signer entries.");
    }

    auto err = sortAndValidateSigners (signers, srcAddressID);
    if (RPC::contains_error (err))
        return err;

    std::pair <Json::Value, Transaction::pointer> txn =
        transactionConstructImpl (stpTrans, ledger->rules(), app);

    if (!txn.second)
        return txn.first;

    try
    {
        processTransaction (
            txn.second, isUnlimited (role), true, failType);
    }
    catch (std::exception&)
    {
        return RPC::make_error (rpcINTERNAL,
            "Exception occurred during transaction submission.");
    }

    return transactionFormatResultImpl (txn.second);
}

} 
} 
























