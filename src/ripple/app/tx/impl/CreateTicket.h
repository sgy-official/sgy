

#ifndef RIPPLE_TX_CREATETICKET_H_INCLUDED
#define RIPPLE_TX_CREATETICKET_H_INCLUDED

#include <ripple/app/ledger/Ledger.h>
#include <ripple/app/tx/impl/Transactor.h>
#include <ripple/basics/Log.h>
#include <ripple/protocol/Indexes.h>

namespace ripple {

class CreateTicket
    : public Transactor
{
public:
    explicit CreateTicket (ApplyContext& ctx)
        : Transactor(ctx)
    {
    }

    static
    NotTEC
    preflight (PreflightContext const& ctx);

    TER doApply () override;
};

}

#endif








