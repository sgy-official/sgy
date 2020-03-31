

#ifndef RIPPLE_TX_APPLYSTEPS_H_INCLUDED
#define RIPPLE_TX_APPLYSTEPS_H_INCLUDED

#include <ripple/ledger/ApplyViewImpl.h>
#include <ripple/beast/utility/Journal.h>

namespace ripple {

class Application;
class STTx;


inline
bool
isTecClaimHardFail(TER ter, ApplyFlags flags)
{
    return isTecClaim(ter) && !(flags & tapRETRY);
}


struct PreflightResult
{
public:
    STTx const& tx;
    Rules const rules;
    ApplyFlags const flags;
    beast::Journal const j;

    NotTEC const ter;

    template<class Context>
    PreflightResult(Context const& ctx_,
        NotTEC ter_)
        : tx(ctx_.tx)
        , rules(ctx_.rules)
        , flags(ctx_.flags)
        , j(ctx_.j)
        , ter(ter_)
    {
    }

    PreflightResult(PreflightResult const&) = default;
    PreflightResult& operator=(PreflightResult const&) = delete;
};


struct PreclaimResult
{
public:
    ReadView const& view;
    STTx const& tx;
    ApplyFlags const flags;
    beast::Journal const j;

    TER const ter;
    bool const likelyToClaimFee;

    template<class Context>
    PreclaimResult(Context const& ctx_, TER ter_)
        : view(ctx_.view)
        , tx(ctx_.tx)
        , flags(ctx_.flags)
        , j(ctx_.j)
        , ter(ter_)
        , likelyToClaimFee(ter == tesSUCCESS
            || isTecClaimHardFail(ter, flags))
    {
    }

    PreclaimResult(PreclaimResult const&) = default;
    PreclaimResult& operator=(PreclaimResult const&) = delete;
};


struct TxConsequences
{
    enum ConsequenceCategory
    {
        normal = 0,
        blocker
    };

    ConsequenceCategory const category;
    XRPAmount const fee;
    XRPAmount const potentialSpend;

    TxConsequences(ConsequenceCategory const category_,
        XRPAmount const fee_, XRPAmount const spend_)
        : category(category_)
        , fee(fee_)
        , potentialSpend(spend_)
    {
    }

    TxConsequences(TxConsequences const&) = default;
    TxConsequences& operator=(TxConsequences const&) = delete;
    TxConsequences(TxConsequences&&) = default;
    TxConsequences& operator=(TxConsequences&&) = delete;

};


PreflightResult
preflight(Application& app, Rules const& rules,
    STTx const& tx, ApplyFlags flags,
        beast::Journal j);


PreclaimResult
preclaim(PreflightResult const& preflightResult,
    Application& app, OpenView const& view);


std::uint64_t
calculateBaseFee(ReadView const& view,
    STTx const& tx);


TxConsequences
calculateConsequences(PreflightResult const& preflightResult);


std::pair<TER, bool>
doApply(PreclaimResult const& preclaimResult,
    Application& app, OpenView& view);

}

#endif








