

#ifndef RIPPLE_TX_CREATEOFFER_H_INCLUDED
#define RIPPLE_TX_CREATEOFFER_H_INCLUDED

#include <ripple/app/tx/impl/OfferStream.h>
#include <ripple/app/tx/impl/Taker.h>
#include <ripple/app/tx/impl/Transactor.h>
#include <utility>

namespace ripple {

class PaymentSandbox;
class Sandbox;


class CreateOffer
    : public Transactor
{
public:
    
    explicit CreateOffer (ApplyContext& ctx)
        : Transactor(ctx)
        , stepCounter_ (1000, j_)
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

    
    void
    preCompute() override;

    
    TER
    doApply() override;

private:
    std::pair<TER, bool>
    applyGuts (Sandbox& view, Sandbox& view_cancel);

    static
    TER
    checkAcceptAsset(ReadView const& view,
        ApplyFlags const flags, AccountID const id,
            beast::Journal const j, Issue const& issue);

    bool
    dry_offer (ApplyView& view, Offer const& offer);

    static
    std::pair<bool, Quality>
    select_path (
        bool have_direct, OfferStream const& direct,
        bool have_bridge, OfferStream const& leg1, OfferStream const& leg2);

    std::pair<TER, Amounts>
    bridged_cross (
        Taker& taker,
        ApplyView& view,
        ApplyView& view_cancel,
        NetClock::time_point const when);

    std::pair<TER, Amounts>
    direct_cross (
        Taker& taker,
        ApplyView& view,
        ApplyView& view_cancel,
        NetClock::time_point const when);

    static
    bool
    step_account (OfferStream& stream, Taker const& taker);

    bool
    reachedOfferCrossingLimit (Taker const& taker) const;

    std::pair<TER, Amounts>
    takerCross (
        Sandbox& sb,
        Sandbox& sbCancel,
        Amounts const& takerAmount);

    std::pair<TER, Amounts>
    flowCross (
        PaymentSandbox& psb,
        PaymentSandbox& psbCancel,
        Amounts const& takerAmount);

    std::pair<TER, Amounts>
    cross (
        Sandbox& sb,
        Sandbox& sbCancel,
        Amounts const& takerAmount);

    static
    std::string
    format_amount (STAmount const& amount);

private:
    CrossType cross_type_;

    OfferStream::StepCounter stepCounter_;
};

}

#endif








