

#ifndef RIPPLE_TX_ESCROW_H_INCLUDED
#define RIPPLE_TX_ESCROW_H_INCLUDED

#include <ripple/app/tx/impl/Transactor.h>

namespace ripple {

class EscrowCreate
    : public Transactor
{
public:
    explicit
    EscrowCreate (ApplyContext& ctx)
        : Transactor(ctx)
    {
    }

    static
    XRPAmount
    calculateMaxSpend(STTx const& tx);

    static
    NotTEC
    preflight (PreflightContext const& ctx);

    TER
    doApply() override;
};


class EscrowFinish
    : public Transactor
{
public:
    explicit
    EscrowFinish (ApplyContext& ctx)
        : Transactor(ctx)
    {
    }

    static
    NotTEC
    preflight (PreflightContext const& ctx);

    static
    std::uint64_t
    calculateBaseFee (
        ReadView const& view,
        STTx const& tx);

    TER
    doApply() override;
};


class EscrowCancel
    : public Transactor
{
public:
    explicit
    EscrowCancel (ApplyContext& ctx)
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








