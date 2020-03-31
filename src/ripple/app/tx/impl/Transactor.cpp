

#include <ripple/app/main/Application.h>
#include <ripple/app/misc/LoadFeeTrack.h>
#include <ripple/app/tx/apply.h>
#include <ripple/app/tx/impl/Transactor.h>
#include <ripple/app/tx/impl/SignerEntries.h>
#include <ripple/basics/contract.h>
#include <ripple/basics/Log.h>
#include <ripple/core/Config.h>
#include <ripple/json/to_string.h>
#include <ripple/ledger/View.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/Indexes.h>
#include <ripple/protocol/UintTypes.h>
#include <ripple/protocol/Protocol.h>
#include <ripple/protocol/STAccount.h>

namespace ripple {


NotTEC
preflight0(PreflightContext const& ctx)
{
    auto const txID = ctx.tx.getTransactionID();

    if (txID == beast::zero)
    {
        JLOG(ctx.j.warn()) <<
            "applyTransaction: transaction id may not be zero";
        return temINVALID;
    }

    return tesSUCCESS;
}


NotTEC
preflight1 (PreflightContext const& ctx)
{
    auto const ret = preflight0(ctx);
    if (!isTesSuccess(ret))
        return ret;

    auto const id = ctx.tx.getAccountID(sfAccount);
    if (id == beast::zero)
    {
        JLOG(ctx.j.warn()) << "preflight1: bad account id";
        return temBAD_SRC_ACCOUNT;
    }

    auto const fee = ctx.tx.getFieldAmount (sfFee);
    if (!fee.native () || fee.negative () || !isLegalAmount (fee.xrp ()))
    {
        JLOG(ctx.j.debug()) << "preflight1: invalid fee";
        return temBAD_FEE;
    }

    auto const spk = ctx.tx.getSigningPubKey();

    if (!spk.empty () && !publicKeyType (makeSlice (spk)))
    {
        JLOG(ctx.j.debug()) << "preflight1: invalid signing key";
        return temBAD_SIGNATURE;
    }

    return tesSUCCESS;
}


NotTEC
preflight2 (PreflightContext const& ctx)
{
    auto const sigValid = checkValidity(ctx.app.getHashRouter(),
        ctx.tx, ctx.rules, ctx.app.config());
    if (sigValid.first == Validity::SigBad)
    {
        JLOG(ctx.j.debug()) <<
            "preflight2: bad signature. " << sigValid.second;
        return temINVALID;
    }
    return tesSUCCESS;
}


PreflightContext::PreflightContext(Application& app_, STTx const& tx_,
    Rules const& rules_, ApplyFlags flags_,
        beast::Journal j_)
    : app(app_)
    , tx(tx_)
    , rules(rules_)
    , flags(flags_)
    , j(j_)
{
}


Transactor::Transactor(
    ApplyContext& ctx)
    : ctx_ (ctx)
    , j_ (ctx.journal)
{
}

std::uint64_t Transactor::calculateBaseFee (
    ReadView const& view,
    STTx const& tx)
{

    std::uint64_t baseFee = view.fees().units;

    std::uint32_t signerCount = 0;
    if (tx.isFieldPresent (sfSigners))
        signerCount = tx.getFieldArray (sfSigners).size();

    return baseFee + (signerCount * baseFee);
}

XRPAmount
Transactor::calculateFeePaid(STTx const& tx)
{
    return tx[sfFee].xrp();
}

XRPAmount
Transactor::minimumFee (Application& app, std::uint64_t baseFee,
    Fees const& fees, ApplyFlags flags)
{
    return scaleFeeLoad (baseFee, app.getFeeTrack (),
        fees, flags & tapUNLIMITED);
}

XRPAmount
Transactor::calculateMaxSpend(STTx const& tx)
{
    return beast::zero;
}

TER
Transactor::checkFee (PreclaimContext const& ctx,
    std::uint64_t baseFee)
{
    auto const feePaid = calculateFeePaid(ctx.tx);
    if (!isLegalAmount (feePaid) || feePaid < beast::zero)
        return temBAD_FEE;

    auto const feeDue = minimumFee(ctx.app,
        baseFee, ctx.view.fees(), ctx.flags);

    if (ctx.view.open() && feePaid < feeDue)
    {
        JLOG(ctx.j.trace()) << "Insufficient fee paid: " <<
            to_string (feePaid) << "/" << to_string (feeDue);
        return telINSUF_FEE_P;
    }

    if (feePaid == beast::zero)
        return tesSUCCESS;

    auto const id = ctx.tx.getAccountID(sfAccount);
    auto const sle = ctx.view.read(
        keylet::account(id));
    auto const balance = (*sle)[sfBalance].xrp();

    if (balance < feePaid)
    {
        JLOG(ctx.j.trace()) << "Insufficient balance:" <<
            " balance=" << to_string(balance) <<
            " paid=" << to_string(feePaid);

        if ((balance > beast::zero) && !ctx.view.open())
        {
            return tecINSUFF_FEE;
        }

        return terINSUF_FEE_B;
    }

    return tesSUCCESS;
}

TER Transactor::payFee ()
{
    auto const feePaid = calculateFeePaid(ctx_.tx);

    auto const sle = view().peek(
        keylet::account(account_));


    mSourceBalance -= feePaid;
    sle->setFieldAmount (sfBalance, mSourceBalance);


    return tesSUCCESS;
}

NotTEC
Transactor::checkSeq (PreclaimContext const& ctx)
{
    auto const id = ctx.tx.getAccountID(sfAccount);

    auto const sle = ctx.view.read(
        keylet::account(id));

    if (!sle)
    {
        JLOG(ctx.j.trace()) <<
            "applyTransaction: delay: source account does not exist " <<
            toBase58(ctx.tx.getAccountID(sfAccount));
        return terNO_ACCOUNT;
    }

    std::uint32_t const t_seq = ctx.tx.getSequence ();
    std::uint32_t const a_seq = sle->getFieldU32 (sfSequence);

    if (t_seq != a_seq)
    {
        if (a_seq < t_seq)
        {
            JLOG(ctx.j.trace()) <<
                "applyTransaction: has future sequence number " <<
                "a_seq=" << a_seq << " t_seq=" << t_seq;
            return terPRE_SEQ;
        }

        if (ctx.view.txExists(ctx.tx.getTransactionID ()))
            return tefALREADY;

        JLOG(ctx.j.trace()) << "applyTransaction: has past sequence number " <<
            "a_seq=" << a_seq << " t_seq=" << t_seq;
        return tefPAST_SEQ;
    }

    if (ctx.tx.isFieldPresent (sfAccountTxnID) &&
            (sle->getFieldH256 (sfAccountTxnID) != ctx.tx.getFieldH256 (sfAccountTxnID)))
        return tefWRONG_PRIOR;

    if (ctx.tx.isFieldPresent (sfLastLedgerSequence) &&
            (ctx.view.seq() > ctx.tx.getFieldU32 (sfLastLedgerSequence)))
        return tefMAX_LEDGER;

    return tesSUCCESS;
}

void
Transactor::setSeq ()
{
    auto const sle = view().peek(
        keylet::account(account_));

    std::uint32_t const t_seq = ctx_.tx.getSequence ();

    sle->setFieldU32 (sfSequence, t_seq + 1);

    if (sle->isFieldPresent (sfAccountTxnID))
        sle->setFieldH256 (sfAccountTxnID, ctx_.tx.getTransactionID ());
}

void Transactor::preCompute ()
{
    account_ = ctx_.tx.getAccountID(sfAccount);
    assert(account_ != beast::zero);
}

TER Transactor::apply ()
{
    preCompute();

    auto const sle = view().peek (keylet::account(account_));

    assert(sle != nullptr || account_ == beast::zero);

    if (sle)
    {
        mPriorBalance   = STAmount ((*sle)[sfBalance]).xrp ();
        mSourceBalance  = mPriorBalance;

        setSeq();

        auto result = payFee ();

        if (result  != tesSUCCESS)
            return result;

        view().update (sle);
    }

    return doApply ();
}

NotTEC
Transactor::checkSign (PreclaimContext const& ctx)
{
    if (ctx.view.rules().enabled(featureMultiSign))
    {
        if (ctx.tx.getSigningPubKey().empty ())
            return checkMultiSign (ctx);
    }

    return checkSingleSign (ctx);
}

NotTEC
Transactor::checkSingleSign (PreclaimContext const& ctx)
{
    auto const pkSigner = ctx.tx.getSigningPubKey();
    if (!publicKeyType(makeSlice(pkSigner)))
    {
        JLOG(ctx.j.trace()) <<
            "checkSingleSign: signing public key type is unknown";
        return tefBAD_AUTH; 
    }

    auto const idSigner = calcAccountID(PublicKey(makeSlice(pkSigner)));
    auto const idAccount = ctx.tx.getAccountID(sfAccount);
    auto const sleAccount = ctx.view.read(keylet::account(idAccount));
    bool const isMasterDisabled = sleAccount->isFlag(lsfDisableMaster);

    if (ctx.view.rules().enabled(fixMasterKeyAsRegularKey))
    {

        if ((*sleAccount)[~sfRegularKey] == idSigner)
        {
            return tesSUCCESS;
        }

        if (!isMasterDisabled && idAccount == idSigner)
        {
            return tesSUCCESS;
        }

        if (isMasterDisabled && idAccount == idSigner)
        {
            return tefMASTER_DISABLED;
        }

        return tefBAD_AUTH;

    }

    if (idSigner == idAccount)
    {
        if (isMasterDisabled)
            return tefMASTER_DISABLED;
    }
    else if ((*sleAccount)[~sfRegularKey] == idSigner)
    {
    }
    else if (sleAccount->isFieldPresent(sfRegularKey))
    {
        JLOG(ctx.j.trace()) <<
            "checkSingleSign: Not authorized to use account.";
        return tefBAD_AUTH;
    }
    else
    {
        JLOG(ctx.j.trace()) <<
            "checkSingleSign: Not authorized to use account.";
        return tefBAD_AUTH_MASTER;
    }

    return tesSUCCESS;
}

NotTEC
Transactor::checkMultiSign (PreclaimContext const& ctx)
{
    auto const id = ctx.tx.getAccountID(sfAccount);
    std::shared_ptr<STLedgerEntry const> sleAccountSigners =
        ctx.view.read (keylet::signers(id));
    if (!sleAccountSigners)
    {
        JLOG(ctx.j.trace()) <<
            "applyTransaction: Invalid: Not a multi-signing account.";
        return tefNOT_MULTI_SIGNING;
    }

    assert (sleAccountSigners->isFieldPresent (sfSignerListID));
    assert (sleAccountSigners->getFieldU32 (sfSignerListID) == 0);

    auto accountSigners =
        SignerEntries::deserialize (*sleAccountSigners, ctx.j, "ledger");
    if (accountSigners.second != tesSUCCESS)
        return accountSigners.second;

    STArray const& txSigners (ctx.tx.getFieldArray (sfSigners));


    std::uint32_t weightSum = 0;
    auto iter = accountSigners.first.begin ();
    for (auto const& txSigner : txSigners)
    {
        AccountID const txSignerAcctID = txSigner.getAccountID (sfAccount);

        while (iter->account < txSignerAcctID)
        {
            if (++iter == accountSigners.first.end ())
            {
                JLOG(ctx.j.trace()) <<
                    "applyTransaction: Invalid SigningAccount.Account.";
                return tefBAD_SIGNATURE;
            }
        }
        if (iter->account != txSignerAcctID)
        {
            JLOG(ctx.j.trace()) <<
                "applyTransaction: Invalid SigningAccount.Account.";
            return tefBAD_SIGNATURE;
        }

        auto const spk = txSigner.getFieldVL (sfSigningPubKey);

        if (!publicKeyType (makeSlice(spk)))
        {
            JLOG(ctx.j.trace()) <<
                "checkMultiSign: signing public key type is unknown";
            return tefBAD_SIGNATURE;
        }

        AccountID const signingAcctIDFromPubKey =
            calcAccountID(PublicKey (makeSlice(spk)));


        auto sleTxSignerRoot =
            ctx.view.read (keylet::account(txSignerAcctID));

        if (signingAcctIDFromPubKey == txSignerAcctID)
        {
            if (sleTxSignerRoot)
            {
                std::uint32_t const signerAccountFlags =
                    sleTxSignerRoot->getFieldU32 (sfFlags);

                if (signerAccountFlags & lsfDisableMaster)
                {
                    JLOG(ctx.j.trace()) <<
                        "applyTransaction: Signer:Account lsfDisableMaster.";
                    return tefMASTER_DISABLED;
                }
            }
        }
        else
        {
            if (!sleTxSignerRoot)
            {
                JLOG(ctx.j.trace()) <<
                    "applyTransaction: Non-phantom signer lacks account root.";
                return tefBAD_SIGNATURE;
            }

            if (!sleTxSignerRoot->isFieldPresent (sfRegularKey))
            {
                JLOG(ctx.j.trace()) <<
                    "applyTransaction: Account lacks RegularKey.";
                return tefBAD_SIGNATURE;
            }
            if (signingAcctIDFromPubKey !=
                sleTxSignerRoot->getAccountID (sfRegularKey))
            {
                JLOG(ctx.j.trace()) <<
                    "applyTransaction: Account doesn't match RegularKey.";
                return tefBAD_SIGNATURE;
            }
        }
        weightSum += iter->weight;
    }

    if (weightSum < sleAccountSigners->getFieldU32 (sfSignerQuorum))
    {
        JLOG(ctx.j.trace()) <<
            "applyTransaction: Signers failed to meet quorum.";
        return tefBAD_QUORUM;
    }

    return tesSUCCESS;
}


static
void removeUnfundedOffers (ApplyView& view, std::vector<uint256> const& offers, beast::Journal viewJ)
{
    int removed = 0;

    for (auto const& index : offers)
    {
        if (auto const sleOffer = view.peek (keylet::offer (index)))
        {
            offerDelete (view, sleOffer, viewJ);
            if (++removed == unfundedOfferRemoveLimit)
                return;
        }
    }
}


XRPAmount
Transactor::reset(XRPAmount fee)
{
    ctx_.discard();

    auto const txnAcct = view().peek(
        keylet::account(ctx_.tx.getAccountID(sfAccount)));

    auto const balance = txnAcct->getFieldAmount (sfBalance).xrp ();

    assert(balance != beast::zero && (!view().open() || balance >= fee));

    if (fee > balance)
        fee = balance;

    txnAcct->setFieldAmount (sfBalance, balance - fee);
    txnAcct->setFieldU32 (sfSequence, ctx_.tx.getSequence() + 1);

    view().update (txnAcct);

    return fee;
}

std::pair<TER, bool>
Transactor::operator()()
{
    JLOG(j_.trace()) << "apply: " << ctx_.tx.getTransactionID ();

#ifdef BEAST_DEBUG
    {
        Serializer ser;
        ctx_.tx.add (ser);
        SerialIter sit(ser.slice());
        STTx s2 (sit);

        if (! s2.isEquivalent(ctx_.tx))
        {
            JLOG(j_.fatal()) <<
                "Transaction serdes mismatch";
            JLOG(j_.info()) << to_string(ctx_.tx.getJson (JsonOptions::none));
            JLOG(j_.fatal()) << s2.getJson (JsonOptions::none);
            assert (false);
        }
    }
#endif

    auto result = ctx_.preclaimResult;
    if (result == tesSUCCESS)
        result = apply();

    assert(result != temUNKNOWN);

    if (auto stream = j_.trace())
        stream << "preclaim result: " << transToken(result);

    bool applied = isTesSuccess (result);
    auto fee = ctx_.tx.getFieldAmount(sfFee).xrp ();

    if (ctx_.size() > oversizeMetaDataCap)
        result = tecOVERSIZE;

    if ((result == tecOVERSIZE) || (result == tecKILLED) ||
        (isTecClaimHardFail (result, view().flags())))
    {
        JLOG(j_.trace()) << "reapplying because of " << transToken(result);

        std::vector<uint256> removedOffers;

        if ((result == tecOVERSIZE) || (result == tecKILLED))
        {
            ctx_.visit (
                [&removedOffers](
                    uint256 const& index,
                    bool isDelete,
                    std::shared_ptr <SLE const> const& before,
                    std::shared_ptr <SLE const> const& after)
                {
                    if (isDelete)
                    {
                        assert (before && after);
                        if (before && after &&
                            (before->getType() == ltOFFER) &&
                            (before->getFieldAmount(sfTakerPays) == after->getFieldAmount(sfTakerPays)))
                        {
                            removedOffers.push_back (index);
                        }
                    }
                });
        }

        fee = reset(fee);

        if ((result == tecOVERSIZE) || (result == tecKILLED))
            removeUnfundedOffers (view(), removedOffers, ctx_.app.journal ("View"));

        applied = true;
    }

    if (applied)
    {
        result = ctx_.checkInvariants(result, fee);

        if (result == tecINVARIANT_FAILED)
        {
            fee = reset(fee);

            result = ctx_.checkInvariants(result, fee);
        }

        if (!isTecClaim(result) && !isTesSuccess(result))
            applied = false;
    }

    if (applied)
    {

        if (fee < beast::zero)
            Throw<std::logic_error> ("fee charged is negative!");

        if (!view().open() && fee != beast::zero)
            ctx_.destroyXRP (fee);

        ctx_.apply(result);
    }

    JLOG(j_.trace()) << (applied ? "applied" : "not applied") << transToken(result);

    return { result, applied };
}

}
























