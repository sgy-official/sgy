

#ifndef RIPPLE_TX_CANCELCHECK_H_INCLUDED
#define RIPPLE_TX_CANCELCHECK_H_INCLUDED

#include <ripple/app/tx/impl/Transactor.h>

namespace ripple {

class CancelCheck
    : public Transactor
{
public:
    explicit CancelCheck (ApplyContext& ctx)
        : Transactor (ctx)
    {
    }

    static
    NotTEC
    preflight (PreflightContext const& ctx);

    static
    TER
    preclaim (PreclaimContext const& ctx);

    TER doApply () override;
};

}

#endif








