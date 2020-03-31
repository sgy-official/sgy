

#ifndef RIPPLE_TX_SETACCOUNT_H_INCLUDED
#define RIPPLE_TX_SETACCOUNT_H_INCLUDED

#include <ripple/app/tx/impl/Transactor.h>
#include <ripple/basics/Log.h>
#include <ripple/core/Config.h>
#include <ripple/protocol/Indexes.h>
#include <ripple/protocol/Quality.h>
#include <ripple/protocol/TxFlags.h>

namespace ripple {

class SetAccount
    : public Transactor
{
    static std::size_t const DOMAIN_BYTES_MAX = 256;

public:
    explicit SetAccount (ApplyContext& ctx)
        : Transactor(ctx)
    {
    }

    static
    bool
    affectsSubsequentTransactionAuth(STTx const& tx);

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








