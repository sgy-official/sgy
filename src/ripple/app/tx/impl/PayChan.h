

#ifndef RIPPLE_TX_PAYCHAN_H_INCLUDED
#define RIPPLE_TX_PAYCHAN_H_INCLUDED

#include <ripple/app/tx/impl/Transactor.h>

namespace ripple {

class PayChanCreate
    : public Transactor
{
public:
    explicit
    PayChanCreate (ApplyContext& ctx)
        : Transactor(ctx)
    {
    }

    static
    NotTEC
    preflight (PreflightContext const& ctx);

    static
    TER
    preclaim(PreclaimContext const &ctx);

    TER
    doApply() override;
};


class PayChanFund
    : public Transactor
{
public:
    explicit
    PayChanFund (ApplyContext& ctx)
        : Transactor(ctx)
    {
    }

    static
    NotTEC
    preflight (PreflightContext const& ctx);

    TER
    doApply() override;
};


class PayChanClaim
    : public Transactor
{
public:
    explicit
    PayChanClaim (ApplyContext& ctx)
        : Transactor(ctx)
    {
    }

    static
    NotTEC
    preflight (PreflightContext const& ctx);

    TER
    doApply() override;
};

}  

#endif








