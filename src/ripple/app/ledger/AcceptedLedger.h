

#ifndef RIPPLE_APP_LEDGER_ACCEPTEDLEDGER_H_INCLUDED
#define RIPPLE_APP_LEDGER_ACCEPTEDLEDGER_H_INCLUDED

#include <ripple/app/ledger/AcceptedLedgerTx.h>
#include <ripple/protocol/AccountID.h>

namespace ripple {



class AcceptedLedger
{
public:
    using pointer        = std::shared_ptr<AcceptedLedger>;
    using ret            = const pointer&;
    using map_t          = std::map<int, AcceptedLedgerTx::pointer>;
    using value_type     = map_t::value_type;
    using const_iterator = map_t::const_iterator;

public:
    std::shared_ptr<ReadView const> const& getLedger () const
    {
        return mLedger;
    }
    const map_t& getMap () const
    {
        return mMap;
    }

    int getTxnCount () const
    {
        return mMap.size ();
    }

    AcceptedLedgerTx::pointer getTxn (int) const;

    AcceptedLedger (
        std::shared_ptr<ReadView const> const& ledger,
        AccountIDCache const& accountCache, Logs& logs);

private:
    void insert (AcceptedLedgerTx::ref);

    std::shared_ptr<ReadView const> mLedger;
    map_t mMap;
};

} 

#endif








