

#ifndef RIPPLE_TX_CREATECHECK_H_INCLUDED
#define RIPPLE_TX_CREATECHECK_H_INCLUDED

#include <ripple/app/tx/impl/Transactor.h>

namespace ripple {

class CreateCheck
    : public Transactor
{
public:
    explicit CreateCheck (ApplyContext& ctx)
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








