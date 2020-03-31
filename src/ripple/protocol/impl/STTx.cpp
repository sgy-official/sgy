

#include <ripple/protocol/STTx.h>
#include <ripple/basics/contract.h>
#include <ripple/basics/Log.h>
#include <ripple/basics/safe_cast.h>
#include <ripple/basics/StringUtilities.h>
#include <ripple/protocol/HashPrefix.h>
#include <ripple/protocol/jss.h>
#include <ripple/protocol/Protocol.h>
#include <ripple/protocol/PublicKey.h>
#include <ripple/protocol/Sign.h>
#include <ripple/protocol/STAccount.h>
#include <ripple/protocol/STArray.h>
#include <ripple/protocol/TxFlags.h>
#include <ripple/protocol/UintTypes.h>
#include <ripple/json/to_string.h>
#include <boost/format.hpp>
#include <array>
#include <memory>
#include <type_traits>
#include <utility>

namespace ripple {

static
auto getTxFormat (TxType type)
{
    auto format = TxFormats::getInstance().findByType (type);

    if (format == nullptr)
    {
        Throw<std::runtime_error> (
            "Invalid transaction type " +
            std::to_string (
                safe_cast<std::underlying_type_t<TxType>>(type)));
    }

    return format;
}

STTx::STTx (STObject&& object) noexcept (false)
    : STObject (std::move (object))
{
    tx_type_ = safe_cast<TxType> (getFieldU16 (sfTransactionType));
    applyTemplate (getTxFormat (tx_type_)->getSOTemplate());  
    tid_ = getHash(HashPrefix::transactionID);
}

STTx::STTx (SerialIter& sit) noexcept (false)
    : STObject (sfTransaction)
{
    int length = sit.getBytesLeft ();

    if ((length < txMinSizeBytes) || (length > txMaxSizeBytes))
        Throw<std::runtime_error> ("Transaction length invalid");

    if (set (sit))
        Throw<std::runtime_error> ("Transaction contains an object terminator");

    tx_type_ = safe_cast<TxType> (getFieldU16 (sfTransactionType));

    applyTemplate (getTxFormat (tx_type_)->getSOTemplate());  
    tid_ = getHash(HashPrefix::transactionID);
}

STTx::STTx (
        TxType type,
        std::function<void(STObject&)> assembler)
    : STObject (sfTransaction)
{
    auto format = getTxFormat (type);

    set (format->getSOTemplate());
    setFieldU16 (sfTransactionType, format->getType ());

    assembler (*this);

    tx_type_ = safe_cast<TxType>(getFieldU16 (sfTransactionType));

    if (tx_type_ != type)
        LogicError ("Transaction type was mutated during assembly");

    tid_ = getHash(HashPrefix::transactionID);
}

std::string
STTx::getFullText () const
{
    std::string ret = "\"";
    ret += to_string (getTransactionID ());
    ret += "\" = {";
    ret += STObject::getFullText ();
    ret += "}";
    return ret;
}

boost::container::flat_set<AccountID>
STTx::getMentionedAccounts () const
{
    boost::container::flat_set<AccountID> list;

    for (auto const& it : *this)
    {
        if (auto sa = dynamic_cast<STAccount const*> (&it))
        {
            assert(! sa->isDefault());
            if (! sa->isDefault())
                list.insert(sa->value());
        }
        else if (auto sa = dynamic_cast<STAmount const*> (&it))
        {
            auto const& issuer = sa->getIssuer ();
            if (! isXRP (issuer))
                list.insert(issuer);
        }
    }

    return list;
}

static Blob getSigningData (STTx const& that)
{
    Serializer s;
    s.add32 (HashPrefix::txSign);
    that.addWithoutSigningFields (s);
    return s.getData();
}

uint256
STTx::getSigningHash () const
{
    return STObject::getSigningHash (HashPrefix::txSign);
}

Blob STTx::getSignature () const
{
    try
    {
        return getFieldVL (sfTxnSignature);
    }
    catch (std::exception const&)
    {
        return Blob ();
    }
}

void STTx::sign (
    PublicKey const& publicKey,
    SecretKey const& secretKey)
{
    auto const data = getSigningData (*this);

    auto const sig = ripple::sign (
        publicKey,
        secretKey,
        makeSlice(data));

    setFieldVL (sfTxnSignature, sig);
    tid_ = getHash(HashPrefix::transactionID);
}

std::pair<bool, std::string> STTx::checkSign(bool allowMultiSign) const
{
    std::pair<bool, std::string> ret {false, ""};
    try
    {
        if (allowMultiSign)
        {
            Blob const& signingPubKey = getFieldVL (sfSigningPubKey);
            ret = signingPubKey.empty () ?
                checkMultiSign () : checkSingleSign ();
        }
        else
        {
            ret = checkSingleSign ();
        }
    }
    catch (std::exception const&)
    {
        ret = {false, "Internal signature check failure."};
    }
    return ret;
}

Json::Value STTx::getJson (JsonOptions) const
{
    Json::Value ret = STObject::getJson (JsonOptions::none);
    ret[jss::hash] = to_string (getTransactionID ());
    return ret;
}

Json::Value STTx::getJson (JsonOptions options, bool binary) const
{
    if (binary)
    {
        Json::Value ret;
        Serializer s = STObject::getSerializer ();
        ret[jss::tx] = strHex (s.peekData ());
        ret[jss::hash] = to_string (getTransactionID ());
        return ret;
    }
    return getJson(options);
}

std::string const&
STTx::getMetaSQLInsertReplaceHeader ()
{
    static std::string const sql = "INSERT OR REPLACE INTO Transactions "
        "(TransID, TransType, FromAcct, FromSeq, LedgerSeq, Status, RawTxn, TxnMeta)"
        " VALUES ";

    return sql;
}

std::string STTx::getMetaSQL (std::uint32_t inLedger,
                                               std::string const& escapedMetaData) const
{
    Serializer s;
    add (s);
    return getMetaSQL (s, inLedger, txnSqlValidated, escapedMetaData);
}

std::string
STTx::getMetaSQL (Serializer rawTxn,
    std::uint32_t inLedger, char status, std::string const& escapedMetaData) const
{
    static boost::format bfTrans ("('%s', '%s', '%s', '%d', '%d', '%c', %s, %s)");
    std::string rTxn = sqlEscape (rawTxn.peekData ());

    auto format = TxFormats::getInstance().findByType (tx_type_);
    assert (format != nullptr);

    return str (boost::format (bfTrans)
                % to_string (getTransactionID ()) % format->getName ()
                % toBase58(getAccountID(sfAccount))
                % getSequence () % inLedger % status % rTxn % escapedMetaData);
}

std::pair<bool, std::string> STTx::checkSingleSign () const
{
    if (isFieldPresent (sfSigners))
        return {false, "Cannot both single- and multi-sign."};

    bool validSig = false;
    try
    {
        bool const fullyCanonical = (getFlags() & tfFullyCanonicalSig);
        auto const spk = getFieldVL (sfSigningPubKey);

        if (publicKeyType (makeSlice(spk)))
        {
            Blob const signature = getFieldVL (sfTxnSignature);
            Blob const data = getSigningData (*this);

            validSig = verify (
                PublicKey (makeSlice(spk)),
                makeSlice(data),
                makeSlice(signature),
                fullyCanonical);
        }
    }
    catch (std::exception const&)
    {
        validSig = false;
    }
    if (validSig == false)
        return {false, "Invalid signature."};

    return {true, ""};
}

std::pair<bool, std::string> STTx::checkMultiSign () const
{
    if (!isFieldPresent (sfSigners))
        return {false, "Empty SigningPubKey."};

    if (isFieldPresent (sfTxnSignature))
        return {false, "Cannot both single- and multi-sign."};

    STArray const& signers {getFieldArray (sfSigners)};

    if (signers.size() < minMultiSigners || signers.size() > maxMultiSigners)
        return {false, "Invalid Signers array size."};

    Serializer const dataStart {startMultiSigningData (*this)};

    auto const txnAccountID = getAccountID (sfAccount);

    bool const fullyCanonical = (getFlags() & tfFullyCanonicalSig);

    AccountID lastAccountID (beast::zero);

    for (auto const& signer : signers)
    {
        auto const accountID = signer.getAccountID (sfAccount);

        if (accountID == txnAccountID)
            return {false, "Invalid multisigner."};

        if (lastAccountID == accountID)
            return {false, "Duplicate Signers not allowed."};

        if (lastAccountID > accountID)
            return {false, "Unsorted Signers array."};

        lastAccountID = accountID;

        bool validSig = false;
        try
        {
            Serializer s = dataStart;
            finishMultiSigningData (accountID, s);

            auto spk = signer.getFieldVL (sfSigningPubKey);

            if (publicKeyType (makeSlice(spk)))
            {
                Blob const signature =
                    signer.getFieldVL (sfTxnSignature);

                validSig = verify (
                    PublicKey (makeSlice(spk)),
                    s.slice(),
                    makeSlice(signature),
                    fullyCanonical);
            }
        }
        catch (std::exception const&)
        {
            validSig = false;
        }
        if (!validSig)
            return {false, std::string("Invalid signature on account ") +
                toBase58(accountID)  + "."};
    }

    return {true, ""};
}


static
bool
isMemoOkay (STObject const& st, std::string& reason)
{
    if (!st.isFieldPresent (sfMemos))
        return true;

    auto const& memos = st.getFieldArray (sfMemos);

    Serializer s (2048);
    memos.add (s);

    if (s.getDataLength () > 1024)
    {
        reason = "The memo exceeds the maximum allowed size.";
        return false;
    }

    for (auto const& memo : memos)
    {
        auto memoObj = dynamic_cast <STObject const*> (&memo);

        if (!memoObj || (memoObj->getFName() != sfMemo))
        {
            reason = "A memo array may contain only Memo objects.";
            return false;
        }

        for (auto const& memoElement : *memoObj)
        {
            auto const& name = memoElement.getFName();

            if (name != sfMemoType &&
                name != sfMemoData &&
                name != sfMemoFormat)
            {
                reason = "A memo may contain only MemoType, MemoData or "
                         "MemoFormat fields.";
                return false;
            }

            auto data = strUnHex (memoElement.getText ());

            if (!data.second)
            {
                reason = "The MemoType, MemoData and MemoFormat fields may "
                         "only contain hex-encoded data.";
                return false;
            }

            if (name == sfMemoData)
                continue;

            static std::array<char, 256> const allowedSymbols = []
            {
                std::array<char, 256> a;
                a.fill(0);

                std::string symbols (
                    "0123456789"
                    "-._~:/?#[]@!$&'()*+,;=%"
                    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                    "abcdefghijklmnopqrstuvwxyz");

                for(char c : symbols)
                    a[c] = 1;
                return a;
            }();

            for (auto c : data.first)
            {
                if (!allowedSymbols[c])
                {
                    reason = "The MemoType and MemoFormat fields may only "
                             "contain characters that are allowed in URLs "
                             "under RFC 3986.";
                    return false;
                }
            }
        }
    }

    return true;
}

static
bool
isAccountFieldOkay (STObject const& st)
{
    for (int i = 0; i < st.getCount(); ++i)
    {
        auto t = dynamic_cast<STAccount const*>(st.peekAtPIndex (i));
        if (t && t->isDefault ())
            return false;
    }

    return true;
}

bool passesLocalChecks (STObject const& st, std::string& reason)
{
    if (!isMemoOkay (st, reason))
        return false;

    if (!isAccountFieldOkay (st))
    {
        reason = "An account field is invalid.";
        return false;
    }

    if (isPseudoTx(st))
    {
        reason = "Cannot submit pseudo transactions.";
        return false;
    }
    return true;
}

std::shared_ptr<STTx const>
sterilize (STTx const& stx)
{
    Serializer s;
    stx.add(s);
    SerialIter sit(s.slice());
    return std::make_shared<STTx const>(std::ref(sit));
}

bool
isPseudoTx(STObject const& tx)
{
    auto t = tx[~sfTransactionType];
    if (!t)
        return false;
    auto tt = safe_cast<TxType>(*t);
    return tt == ttAMENDMENT || tt == ttFEE;
}

} 
























