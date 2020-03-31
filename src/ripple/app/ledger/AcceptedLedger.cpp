

#include <ripple/app/ledger/AcceptedLedger.h>
#include <ripple/basics/Log.h>
#include <ripple/basics/chrono.h>

namespace ripple {

AcceptedLedger::AcceptedLedger (
    std::shared_ptr<ReadView const> const& ledger,
    AccountIDCache const& accountCache, Logs& logs)
    : mLedger (ledger)
{
    for (auto const& item : ledger->txs)
    {
        insert (std::make_shared<AcceptedLedgerTx>(
            ledger, item.first, item.second, accountCache, logs));
    }
}

void AcceptedLedger::insert (AcceptedLedgerTx::ref at)
{
    assert (mMap.find (at->getIndex ()) == mMap.end ());
    mMap.insert (std::make_pair (at->getIndex (), at));
}

AcceptedLedgerTx::pointer AcceptedLedger::getTxn (int i) const
{
    map_t::const_iterator it = mMap.find (i);

    if (it == mMap.end ())
        return AcceptedLedgerTx::pointer ();

    return it->second;
}

} 
























