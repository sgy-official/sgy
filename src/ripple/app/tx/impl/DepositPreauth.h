

#ifndef RIPPLE_TX_DEPOSIT_PREAUTH_H_INCLUDED
#define RIPPLE_TX_DEPOSIT_PREAUTH_H_INCLUDED

#include <ripple/app/tx/impl/Transactor.h>

namespace ripple {

class DepositPreauth
    : public Transactor
{
public:
    explicit DepositPreauth (ApplyContext& ctx)
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









