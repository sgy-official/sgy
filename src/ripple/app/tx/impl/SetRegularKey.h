

#ifndef RIPPLE_TX_SETREGULARKEY_H_INCLUDED
#define RIPPLE_TX_SETREGULARKEY_H_INCLUDED

#include <ripple/app/tx/impl/Transactor.h>
#include <ripple/basics/Log.h>
#include <ripple/protocol/TxFlags.h>
#include <ripple/protocol/UintTypes.h>

namespace ripple {

class SetRegularKey
    : public Transactor
{
public:
    explicit SetRegularKey (ApplyContext& ctx)
        : Transactor(ctx)
    {
    }

    static
    bool
    affectsSubsequentTransactionAuth(STTx const& tx)
    {
        return true;
    }

    static
    NotTEC
    preflight (PreflightContext const& ctx);

    static
    std::uint64_t
    calculateBaseFee (
        ReadView const& view,
        STTx const& tx);

    TER doApply () override;
};

} 

#endif









