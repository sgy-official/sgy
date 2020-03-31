

#ifndef RIPPLE_TX_CANCELOFFER_H_INCLUDED
#define RIPPLE_TX_CANCELOFFER_H_INCLUDED

#include <ripple/app/tx/impl/Transactor.h>
#include <ripple/basics/Log.h>
#include <ripple/protocol/Indexes.h>
#include <ripple/protocol/TxFlags.h>

namespace ripple {

class CancelOffer
    : public Transactor
{
public:
    explicit CancelOffer (ApplyContext& ctx)
        : Transactor(ctx)
    {
    }

    static
    NotTEC
    preflight (PreflightContext const& ctx);

    static
    TER
    preclaim(PreclaimContext const& ctx);

    TER doApply () override;
};

}

#endif








