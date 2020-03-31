

#ifndef RIPPLE_APP_BOOK_TAKER_H_INCLUDED
#define RIPPLE_APP_BOOK_TAKER_H_INCLUDED

#include <ripple/app/tx/impl/Offer.h>
#include <ripple/core/Config.h>
#include <ripple/ledger/View.h>
#include <ripple/protocol/Quality.h>
#include <ripple/protocol/Rate.h>
#include <ripple/protocol/TER.h>
#include <ripple/protocol/TxFlags.h>
#include <ripple/beast/utility/Journal.h>
#include <functional>

namespace ripple {


enum class CrossType
{
    XrpToIou,
    IouToXrp,
    IouToIou
};


class BasicTaker
{
private:
    AccountID account_;
    Quality quality_;
    Quality threshold_;

    bool sell_;

    Amounts const original_;

    Amounts remaining_;

    Issue const& issue_in_;
    Issue const& issue_out_;

    Rate const m_rate_in;
    Rate const m_rate_out;

    CrossType cross_type_;

protected:
    beast::Journal journal_;

    struct Flow
    {
        explicit Flow() = default;

        Amounts order;
        Amounts issuers;

        bool sanity_check () const
        {
            using beast::zero;

            if (isXRP (order.in) && isXRP (order.out))
                return false;

            return order.in >= zero &&
                order.out >= zero &&
                issuers.in >= zero &&
                issuers.out >= zero;
        }
    };

private:
    void
    log_flow (char const* description, Flow const& flow);

    Flow
    flow_xrp_to_iou (Amounts const& offer, Quality quality,
        STAmount const& owner_funds, STAmount const& taker_funds,
        Rate const& rate_out);

    Flow
    flow_iou_to_xrp (Amounts const& offer, Quality quality,
        STAmount const& owner_funds, STAmount const& taker_funds,
        Rate const& rate_in);

    Flow
    flow_iou_to_iou (Amounts const& offer, Quality quality,
        STAmount const& owner_funds, STAmount const& taker_funds,
        Rate const& rate_in, Rate const& rate_out);

    static
    Rate
    effective_rate (Rate const& rate, Issue const &issue,
        AccountID const& from, AccountID const& to);

    Rate
    in_rate (AccountID const& from, AccountID const& to) const
    {
        return effective_rate (m_rate_in, original_.in.issue (), from, to);
    }

    Rate
    out_rate (AccountID const& from, AccountID const& to) const
    {
        return effective_rate (m_rate_out, original_.out.issue (), from, to);
    }

public:
    BasicTaker () = delete;
    BasicTaker (BasicTaker const&) = delete;

    BasicTaker (
        CrossType cross_type, AccountID const& account, Amounts const& amount,
        Quality const& quality, std::uint32_t flags, Rate const& rate_in,
        Rate const& rate_out,
        beast::Journal journal = beast::Journal{beast::Journal::getNullSink()});

    virtual ~BasicTaker () = default;

    
    Amounts
    remaining_offer () const;

    
    Amounts const&
    original_offer () const;

    
    AccountID const&
    account () const noexcept
    {
        return account_;
    }

    
    bool
    reject (Quality const& quality) const noexcept
    {
        return quality < threshold_;
    }

    
    CrossType
    cross_type () const
    {
        return cross_type_;
    }

    
    Issue const&
    issue_in () const
    {
        return issue_in_;
    }

    
    Issue const&
    issue_out () const
    {
        return issue_out_;
    }

    
    bool
    unfunded () const;

    
    bool
    done () const;

    
    BasicTaker::Flow
    do_cross (Amounts offer, Quality quality, AccountID const& owner);

    
    std::pair<BasicTaker::Flow, BasicTaker::Flow>
    do_cross (
        Amounts offer1, Quality quality1, AccountID const& owner1,
        Amounts offer2, Quality quality2, AccountID const& owner2);

    virtual
    STAmount
    get_funds (AccountID const& account, STAmount const& funds) const = 0;
};


class Taker
    : public BasicTaker
{
public:
    Taker () = delete;
    Taker (Taker const&) = delete;

    Taker (CrossType cross_type, ApplyView& view,
        AccountID const& account, Amounts const& offer,
            std::uint32_t flags,
                beast::Journal journal);
    ~Taker () = default;

    void
    consume_offer (Offer& offer, Amounts const& order);

    STAmount
    get_funds (AccountID const& account, STAmount const& funds) const override;

    STAmount const&
    get_xrp_flow () const
    {
        return xrp_flow_;
    }

    std::uint32_t
    get_direct_crossings () const
    {
        return direct_crossings_;
    }

    std::uint32_t
    get_bridge_crossings () const
    {
        return bridge_crossings_;
    }

    
    
    TER
    cross (Offer& offer);

    TER
    cross (Offer& leg1, Offer& leg2);
    

private:
    static
    Rate
    calculateRate (ApplyView const& view,
        AccountID const& issuer,
            AccountID const& account);

    TER
    fill (BasicTaker::Flow const& flow, Offer& offer);

    TER
    fill (
        BasicTaker::Flow const& flow1, Offer& leg1,
        BasicTaker::Flow const& flow2, Offer& leg2);

    TER
    transferXRP (AccountID const& from, AccountID const& to, STAmount const& amount);

    TER
    redeemIOU (AccountID const& account, STAmount const& amount, Issue const& issue);

    TER
    issueIOU (AccountID const& account, STAmount const& amount, Issue const& issue);

private:
    ApplyView& view_;

    STAmount xrp_flow_;

    std::uint32_t direct_crossings_;

    std::uint32_t bridge_crossings_;
};

}

#endif








