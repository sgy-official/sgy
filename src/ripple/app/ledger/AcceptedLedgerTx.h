

#ifndef RIPPLE_APP_LEDGER_ACCEPTEDLEDGERTX_H_INCLUDED
#define RIPPLE_APP_LEDGER_ACCEPTEDLEDGERTX_H_INCLUDED

#include <ripple/app/ledger/Ledger.h>
#include <ripple/protocol/AccountID.h>
#include <boost/container/flat_set.hpp>

namespace ripple {

class Logs;


class AcceptedLedgerTx
{
public:
    using pointer = std::shared_ptr <AcceptedLedgerTx>;
    using ref = const pointer&;

public:
    AcceptedLedgerTx (
        std::shared_ptr<ReadView const> const& ledger,
        std::shared_ptr<STTx const> const&,
        std::shared_ptr<STObject const> const&,
        AccountIDCache const&,
        Logs&);
    AcceptedLedgerTx (
        std::shared_ptr<ReadView const> const&,
        std::shared_ptr<STTx const> const&,
        TER,
        AccountIDCache const&,
        Logs&);

    std::shared_ptr <STTx const> const& getTxn () const
    {
        return mTxn;
    }
    std::shared_ptr <TxMeta> const& getMeta () const
    {
        return mMeta;
    }

    boost::container::flat_set<AccountID> const&
    getAffected() const
    {
        return mAffected;
    }

    TxID getTransactionID () const
    {
        return mTxn->getTransactionID ();
    }
    TxType getTxnType () const
    {
        return mTxn->getTxnType ();
    }
    TER getResult () const
    {
        return mResult;
    }
    std::uint32_t getTxnSeq () const
    {
        return mMeta->getIndex ();
    }

    bool isApplied () const
    {
        return bool(mMeta);
    }
    int getIndex () const
    {
        return mMeta ? mMeta->getIndex () : 0;
    }
    std::string getEscMeta () const;
    Json::Value getJson () const
    {
        return mJson;
    }

private:
    std::shared_ptr<ReadView const> mLedger;
    std::shared_ptr<STTx const> mTxn;
    std::shared_ptr<TxMeta> mMeta;
    TER                             mResult;
    boost::container::flat_set<AccountID> mAffected;
    Blob        mRawMeta;
    Json::Value                     mJson;
    AccountIDCache const& accountCache_;
    Logs& logs_;

    void buildJson ();
};

} 

#endif








