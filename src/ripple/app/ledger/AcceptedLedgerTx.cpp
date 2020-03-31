

#include <ripple/app/main/Application.h>
#include <ripple/app/ledger/AcceptedLedgerTx.h>
#include <ripple/basics/Log.h>
#include <ripple/basics/StringUtilities.h>
#include <ripple/protocol/jss.h>
#include <ripple/protocol/UintTypes.h>

namespace ripple {

AcceptedLedgerTx::AcceptedLedgerTx (
    std::shared_ptr<ReadView const> const& ledger,
    std::shared_ptr<STTx const> const& txn,
    std::shared_ptr<STObject const> const& met,
    AccountIDCache const& accountCache,
    Logs& logs)
    : mLedger (ledger)
    , mTxn (txn)
    , mMeta (std::make_shared<TxMeta> (
        txn->getTransactionID(), ledger->seq(), *met))
    , mAffected (mMeta->getAffectedAccounts (logs.journal("View")))
    , accountCache_ (accountCache)
    , logs_ (logs)
{
    assert (! ledger->open());

    mResult = mMeta->getResultTER ();

    Serializer s;
    met->add(s);
    mRawMeta = std::move (s.modData());

    buildJson ();
}

AcceptedLedgerTx::AcceptedLedgerTx (
    std::shared_ptr<ReadView const> const& ledger,
    std::shared_ptr<STTx const> const& txn,
    TER result,
    AccountIDCache const& accountCache,
    Logs& logs)
    : mLedger (ledger)
    , mTxn (txn)
    , mResult (result)
    , mAffected (txn->getMentionedAccounts ())
    , accountCache_ (accountCache)
    , logs_ (logs)
{
    assert (ledger->open());
    buildJson ();
}

std::string AcceptedLedgerTx::getEscMeta () const
{
    assert (!mRawMeta.empty ());
    return sqlEscape (mRawMeta);
}

void AcceptedLedgerTx::buildJson ()
{
    mJson = Json::objectValue;
    mJson[jss::transaction] = mTxn->getJson (JsonOptions::none);

    if (mMeta)
    {
        mJson[jss::meta] = mMeta->getJson (JsonOptions::none);
        mJson[jss::raw_meta] = strHex (mRawMeta);
    }

    mJson[jss::result] = transHuman (mResult);

    if (! mAffected.empty ())
    {
        Json::Value& affected = (mJson[jss::affected] = Json::arrayValue);
        for (auto const& account: mAffected)
            affected.append (accountCache_.toBase58(account));
    }

    if (mTxn->getTxnType () == ttOFFER_CREATE)
    {
        auto const& account = mTxn->getAccountID(sfAccount);
        auto const amount = mTxn->getFieldAmount (sfTakerGets);

        if (account != amount.issue ().account)
        {
            auto const ownerFunds = accountFunds(*mLedger,
                account, amount, fhIGNORE_FREEZE, logs_.journal ("View"));
            mJson[jss::transaction][jss::owner_funds] = ownerFunds.getText ();
        }
    }
}

} 
























