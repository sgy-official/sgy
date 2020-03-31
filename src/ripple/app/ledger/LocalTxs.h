

#ifndef RIPPLE_APP_LEDGER_LOCALTXS_H_INCLUDED
#define RIPPLE_APP_LEDGER_LOCALTXS_H_INCLUDED

#include <ripple/app/misc/CanonicalTXSet.h>
#include <ripple/ledger/ReadView.h>
#include <memory>

namespace ripple {


class LocalTxs
{
public:
    virtual ~LocalTxs () = default;

    virtual void push_back (LedgerIndex index, std::shared_ptr<STTx const> const& txn) = 0;

    virtual CanonicalTXSet getTxSet () = 0;

    virtual void sweep (ReadView const& view) = 0;

    virtual std::size_t size () = 0;
};

std::unique_ptr<LocalTxs>
make_LocalTxs ();

} 

#endif








