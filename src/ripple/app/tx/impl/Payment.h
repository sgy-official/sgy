

#ifndef RIPPLE_TX_PAYMENT_H_INCLUDED
#define RIPPLE_TX_PAYMENT_H_INCLUDED

#include <ripple/app/paths/RippleCalc.h>
#include <ripple/app/tx/impl/Transactor.h>
#include <ripple/basics/Log.h>
#include <ripple/protocol/TxFlags.h>

namespace ripple {


class Payment
    : public Transactor
{
    
    static std::size_t const MaxPathSize = 6;

    
    static std::size_t const MaxPathLength = 8;

public:
    explicit Payment (ApplyContext& ctx)
        : Transactor(ctx)
    {
    }

    static
    XRPAmount
    calculateMaxSpend(STTx const& tx);

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








