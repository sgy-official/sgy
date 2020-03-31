

#include <ripple/app/tx/impl/Taker.h>
#include <ripple/basics/contract.h>
#include <ripple/basics/Log.h>

namespace ripple {

static
std::string
format_amount (STAmount const& amount)
{
    std::string txt = amount.getText ();
    txt += "/";
    txt += to_string (amount.issue().currency);
    return txt;
}

BasicTaker::BasicTaker (
        CrossType cross_type, AccountID const& account, Amounts const& amount,
        Quality const& quality, std::uint32_t flags, Rate const& rate_in,
        Rate const& rate_out, beast::Journal journal)
    : account_ (account)
    , quality_ (quality)
    , threshold_ (quality_)
    , sell_ (flags & tfSell)
    , original_ (amount)
    , remaining_ (amount)
    , issue_in_ (remaining_.in.issue ())
    , issue_out_ (remaining_.out.issue ())
    , m_rate_in (rate_in)
    , m_rate_out (rate_out)
    , cross_type_ (cross_type)
    , journal_ (journal)
{
    assert (remaining_.in > beast::zero);
    assert (remaining_.out > beast::zero);

    assert (m_rate_in.value != 0);
    assert (m_rate_out.value != 0);

    assert (cross_type != CrossType::XrpToIou ||
        (isXRP (issue_in ()) && !isXRP (issue_out ())));

    assert (cross_type != CrossType::IouToXrp ||
        (!isXRP (issue_in ()) && isXRP (issue_out ())));

    assert (!isXRP (issue_in ()) || !isXRP (issue_out ()));

    if (flags & tfPassive)
        ++threshold_;
}

Rate
BasicTaker::effective_rate (
    Rate const& rate, Issue const &issue,
    AccountID const& from, AccountID const& to)
{
    if (rate != parityRate &&
        from != to &&
        from != issue.account &&
        to != issue.account)
    {
        return rate;
    }

    return parityRate;
}

bool
BasicTaker::unfunded () const
{
    if (get_funds (account(), remaining_.in) > beast::zero)
        return false;

    JLOG(journal_.debug()) << "Unfunded: taker is out of funds.";
    return true;
}

bool
BasicTaker::done () const
{
    if (remaining_.in <= beast::zero)
    {
        JLOG(journal_.debug()) << "Done: all the input currency has been consumed.";
        return true;
    }

    if (!sell_ && (remaining_.out <= beast::zero))
    {
        JLOG(journal_.debug()) << "Done: the desired amount has been received.";
        return true;
    }

    if (unfunded ())
    {
        JLOG(journal_.debug()) << "Done: taker out of funds.";
        return true;
    }

    return false;
}

Amounts
BasicTaker::remaining_offer () const
{
    if (done ())
        return Amounts (remaining_.in.zeroed(), remaining_.out.zeroed());

    if (original_ == remaining_)
       return original_;

    if (sell_)
    {
        assert (remaining_.in > beast::zero);

        return Amounts (remaining_.in, divRound (
            remaining_.in, quality_.rate (), issue_out_, true));
    }

    assert (remaining_.out > beast::zero);

    return Amounts (mulRound (
        remaining_.out, quality_.rate (), issue_in_, true),
        remaining_.out);
}

Amounts const&
BasicTaker::original_offer () const
{
    return original_;
}

static
STAmount
qual_div (STAmount const& amount, Quality const& quality, STAmount const& output)
{
    auto result = divide (amount, quality.rate (), output.issue ());
    return std::min (result, output);
}

static
STAmount
qual_mul (STAmount const& amount, Quality const& quality, STAmount const& output)
{
    auto result = multiply (amount, quality.rate (), output.issue ());
    return std::min (result, output);
}

void
BasicTaker::log_flow (char const* description, Flow const& flow)
{
    auto stream = journal_.debug();
    if (!stream)
        return;

    stream << description;

    if (isXRP (issue_in ()))
        stream << "   order in: " << format_amount (flow.order.in);
    else
        stream << "   order in: " << format_amount (flow.order.in) <<
            " (issuer: " << format_amount (flow.issuers.in) << ")";

    if (isXRP (issue_out ()))
        stream << "  order out: " << format_amount (flow.order.out);
    else
        stream << "  order out: " << format_amount (flow.order.out) <<
            " (issuer: " << format_amount (flow.issuers.out) << ")";
}

BasicTaker::Flow
BasicTaker::flow_xrp_to_iou (
    Amounts const& order, Quality quality,
    STAmount const& owner_funds, STAmount const& taker_funds,
    Rate const& rate_out)
{
    Flow f;
    f.order = order;
    f.issuers.out = multiply (f.order.out, rate_out);

    log_flow ("flow_xrp_to_iou", f);

    if (owner_funds < f.issuers.out)
    {
        f.issuers.out = owner_funds;
        f.order.out = divide (f.issuers.out, rate_out);
        f.order.in = qual_mul (f.order.out, quality, f.order.in);
        log_flow ("(clamped on owner balance)", f);
    }

    if (!sell_ && remaining_.out < f.order.out)
    {
        f.order.out = remaining_.out;
        f.order.in = qual_mul (f.order.out, quality, f.order.in);
        f.issuers.out = multiply (f.order.out, rate_out);
        log_flow ("(clamped on taker output)", f);
    }

    if (taker_funds < f.order.in)
    {
        f.order.in = taker_funds;
        f.order.out = qual_div (f.order.in, quality, f.order.out);
        f.issuers.out = multiply (f.order.out, rate_out);
        log_flow ("(clamped on taker funds)", f);
    }

    if (cross_type_ == CrossType::XrpToIou && (remaining_.in < f.order.in))
    {
        f.order.in = remaining_.in;
        f.order.out = qual_div (f.order.in, quality, f.order.out);
        f.issuers.out = multiply (f.order.out, rate_out);
        log_flow ("(clamped on taker input)", f);
    }

    return f;
}

BasicTaker::Flow
BasicTaker::flow_iou_to_xrp (
    Amounts const& order, Quality quality,
    STAmount const& owner_funds, STAmount const& taker_funds,
    Rate const& rate_in)
{
    Flow f;
    f.order = order;
    f.issuers.in = multiply (f.order.in, rate_in);

    log_flow ("flow_iou_to_xrp", f);

    if (owner_funds < f.order.out)
    {
        f.order.out = owner_funds;
        f.order.in = qual_mul (f.order.out, quality, f.order.in);
        f.issuers.in = multiply (f.order.in, rate_in);
        log_flow ("(clamped on owner funds)", f);
    }

    if (!sell_ && cross_type_ == CrossType::IouToXrp)
    {
        if (remaining_.out < f.order.out)
        {
            f.order.out = remaining_.out;
            f.order.in = qual_mul (f.order.out, quality, f.order.in);
            f.issuers.in = multiply (f.order.in, rate_in);
            log_flow ("(clamped on taker output)", f);
        }
    }

    if (remaining_.in < f.order.in)
    {
        f.order.in = remaining_.in;
        f.issuers.in = multiply (f.order.in, rate_in);
        f.order.out = qual_div (f.order.in, quality, f.order.out);
        log_flow ("(clamped on taker input)", f);
    }

    if (taker_funds < f.issuers.in)
    {
        f.issuers.in = taker_funds;
        f.order.in = divide (f.issuers.in, rate_in);
        f.order.out = qual_div (f.order.in, quality, f.order.out);
        log_flow ("(clamped on taker funds)", f);
    }

    return f;
}

BasicTaker::Flow
BasicTaker::flow_iou_to_iou (
    Amounts const& order, Quality quality,
    STAmount const& owner_funds, STAmount const& taker_funds,
    Rate const& rate_in, Rate const& rate_out)
{
    Flow f;
    f.order = order;
    f.issuers.in = multiply (f.order.in, rate_in);
    f.issuers.out = multiply (f.order.out, rate_out);

    log_flow ("flow_iou_to_iou", f);

    if (owner_funds < f.issuers.out)
    {
        f.issuers.out = owner_funds;
        f.order.out = divide (f.issuers.out, rate_out);
        f.order.in = qual_mul (f.order.out, quality, f.order.in);
        f.issuers.in = multiply (f.order.in, rate_in);
        log_flow ("(clamped on owner funds)", f);
    }

    if (!sell_ && remaining_.out < f.order.out)
    {
        f.order.out = remaining_.out;
        f.order.in = qual_mul (f.order.out, quality, f.order.in);
        f.issuers.out = multiply (f.order.out, rate_out);
        f.issuers.in = multiply (f.order.in, rate_in);
        log_flow ("(clamped on taker output)", f);
    }

    if (remaining_.in < f.order.in)
    {
        f.order.in = remaining_.in;
        f.issuers.in = multiply (f.order.in, rate_in);
        f.order.out = qual_div (f.order.in, quality, f.order.out);
        f.issuers.out = multiply (f.order.out, rate_out);
        log_flow ("(clamped on taker input)", f);
    }

    if (taker_funds < f.issuers.in)
    {
        f.issuers.in = taker_funds;
        f.order.in = divide (f.issuers.in, rate_in);
        f.order.out = qual_div (f.order.in, quality, f.order.out);
        f.issuers.out = multiply (f.order.out, rate_out);
        log_flow ("(clamped on taker funds)", f);
    }

    return f;
}

BasicTaker::Flow
BasicTaker::do_cross (Amounts offer, Quality quality, AccountID const& owner)
{
    auto const owner_funds = get_funds (owner, offer.out);
    auto const taker_funds = get_funds (account (), offer.in);

    Flow result;

    if (cross_type_ == CrossType::XrpToIou)
    {
        result = flow_xrp_to_iou (offer, quality, owner_funds, taker_funds,
            out_rate (owner, account ()));
    }
    else if (cross_type_ == CrossType::IouToXrp)
    {
        result = flow_iou_to_xrp (offer, quality, owner_funds, taker_funds,
            in_rate (owner, account ()));
    }
    else
    {
        result = flow_iou_to_iou (offer, quality, owner_funds, taker_funds,
            in_rate (owner, account ()), out_rate (owner, account ()));
    }

    if (!result.sanity_check ())
        Throw<std::logic_error> ("Computed flow fails sanity check.");

    remaining_.out -= result.order.out;
    remaining_.in -= result.order.in;

    assert (remaining_.in >= beast::zero);

    return result;
}

std::pair<BasicTaker::Flow, BasicTaker::Flow>
BasicTaker::do_cross (
    Amounts offer1, Quality quality1, AccountID const& owner1,
    Amounts offer2, Quality quality2, AccountID const& owner2)
{
    assert (!offer1.in.native ());
    assert (offer1.out.native ());
    assert (offer2.in.native ());
    assert (!offer2.out.native ());

    auto leg1_in_funds = get_funds (account (), offer1.in);

    if (account () == owner1)
    {
        JLOG(journal_.trace()) << "The taker owns the first leg of a bridge.";
        leg1_in_funds = std::max (leg1_in_funds, offer1.in);
    }

    auto leg2_out_funds = get_funds (owner2, offer2.out);

    if (account () == owner2)
    {
        JLOG(journal_.trace()) << "The taker owns the second leg of a bridge.";
        leg2_out_funds = std::max (leg2_out_funds, offer2.out);
    }

    auto xrp_funds = get_funds (owner1, offer1.out);

    if (owner1 == owner2)
    {
        JLOG(journal_.trace()) <<
            "The bridge endpoints are owned by the same account.";
        xrp_funds = std::max (offer1.out, offer2.in);
    }

    if (auto stream = journal_.debug())
    {
        stream << "Available bridge funds:";
        stream << "  leg1 in: " << format_amount (leg1_in_funds);
        stream << " leg2 out: " << format_amount (leg2_out_funds);
        stream << "      xrp: " << format_amount (xrp_funds);
    }

    auto const leg1_rate = in_rate (owner1, account ());
    auto const leg2_rate = out_rate (owner2, account ());

    auto flow1 = flow_iou_to_xrp (offer1, quality1, xrp_funds, leg1_in_funds, leg1_rate);

    if (!flow1.sanity_check ())
        Throw<std::logic_error> ("Computed flow1 fails sanity check.");

    auto flow2 = flow_xrp_to_iou (offer2, quality2, leg2_out_funds, xrp_funds, leg2_rate);

    if (!flow2.sanity_check ())
        Throw<std::logic_error> ("Computed flow2 fails sanity check.");

    if (flow1.order.out < flow2.order.in)
    {
        flow2.order.in = flow1.order.out;
        flow2.order.out = qual_div (flow2.order.in, quality2, flow2.order.out);
        flow2.issuers.out = multiply (flow2.order.out, leg2_rate);
        log_flow ("Balancing: adjusted second leg down", flow2);
    }
    else if (flow1.order.out > flow2.order.in)
    {
        flow1.order.out = flow2.order.in;
        flow1.order.in = qual_mul (flow1.order.out, quality1, flow1.order.in);
        flow1.issuers.in = multiply (flow1.order.in, leg1_rate);
        log_flow ("Balancing: adjusted first leg down", flow2);
    }

    if (flow1.order.out != flow2.order.in)
        Throw<std::logic_error> ("Bridged flow is out of balance.");

    remaining_.out -= flow2.order.out;
    remaining_.in -= flow1.order.in;

    return std::make_pair (flow1, flow2);
}


Taker::Taker (CrossType cross_type, ApplyView& view,
    AccountID const& account, Amounts const& offer,
        std::uint32_t flags,
            beast::Journal journal)
    : BasicTaker (cross_type, account, offer, Quality(offer), flags,
        calculateRate(view, offer.in.getIssuer(), account),
        calculateRate(view, offer.out.getIssuer(), account), journal)
    , view_ (view)
    , xrp_flow_ (0)
    , direct_crossings_ (0)
    , bridge_crossings_ (0)
{
    assert (issue_in () == offer.in.issue ());
    assert (issue_out () == offer.out.issue ());

    if (auto stream = journal_.debug())
    {
        stream << "Crossing as: " << to_string (account);

        if (isXRP (issue_in ()))
            stream << "   Offer in: " << format_amount (offer.in);
        else
            stream << "   Offer in: " << format_amount (offer.in) <<
                " (issuer: " << issue_in ().account << ")";

        if (isXRP (issue_out ()))
            stream << "  Offer out: " << format_amount (offer.out);
        else
            stream << "  Offer out: " << format_amount (offer.out) <<
                " (issuer: " << issue_out ().account << ")";

        stream <<
            "    Balance: " << format_amount (get_funds (account, offer.in));
    }
}

void
Taker::consume_offer (Offer& offer, Amounts const& order)
{
    if (order.in < beast::zero)
        Throw<std::logic_error> ("flow with negative input.");

    if (order.out < beast::zero)
        Throw<std::logic_error> ("flow with negative output.");

    JLOG(journal_.debug()) << "Consuming from offer " << offer;

    if (auto stream = journal_.trace())
    {
        auto const& available = offer.amount ();

        stream << "   in:" << format_amount (available.in);
        stream << "  out:" << format_amount(available.out);
    }

    offer.consume (view_, order);
}

STAmount
Taker::get_funds (AccountID const& account, STAmount const& amount) const
{
    return accountFunds(view_, account, amount, fhZERO_IF_FROZEN, journal_);
}

TER Taker::transferXRP (
    AccountID const& from,
    AccountID const& to,
    STAmount const& amount)
{
    if (!isXRP (amount))
        Throw<std::logic_error> ("Using transferXRP with IOU");

    if (from == to)
        return tesSUCCESS;

    if (amount == beast::zero)
        return tesSUCCESS;

    return ripple::transferXRP (view_, from, to, amount, journal_);
}

TER Taker::redeemIOU (
    AccountID const& account,
    STAmount const& amount,
    Issue const& issue)
{
    if (isXRP (amount))
        Throw<std::logic_error> ("Using redeemIOU with XRP");

    if (account == issue.account)
        return tesSUCCESS;

    if (amount == beast::zero)
        return tesSUCCESS;

    if (get_funds (account, amount) <= beast::zero)
        Throw<std::logic_error> ("redeemIOU has no funds to redeem");

    auto ret = ripple::redeemIOU (view_, account, amount, issue, journal_);

    if (get_funds (account, amount) < beast::zero)
        Throw<std::logic_error> ("redeemIOU redeemed more funds than available");

    return ret;
}

TER Taker::issueIOU (
    AccountID const& account,
    STAmount const& amount,
    Issue const& issue)
{
    if (isXRP (amount))
        Throw<std::logic_error> ("Using issueIOU with XRP");

    if (account == issue.account)
        return tesSUCCESS;

    if (amount == beast::zero)
        return tesSUCCESS;

    return ripple::issueIOU (view_, account, amount, issue, journal_);
}

TER
Taker::fill (BasicTaker::Flow const& flow, Offer& offer)
{
    consume_offer (offer, flow.order);

    TER result = tesSUCCESS;

    if (cross_type () != CrossType::XrpToIou)
    {
        assert (!isXRP (flow.order.in));

        if(result == tesSUCCESS)
            result = redeemIOU (account (), flow.issuers.in, flow.issuers.in.issue ());

        if (result == tesSUCCESS)
            result = issueIOU (offer.owner (), flow.order.in, flow.order.in.issue ());
    }
    else
    {
        assert (isXRP (flow.order.in));

        if (result == tesSUCCESS)
            result = transferXRP (account (), offer.owner (), flow.order.in);
    }

    if (cross_type () != CrossType::IouToXrp)
    {
        assert (!isXRP (flow.order.out));

        if(result == tesSUCCESS)
            result = redeemIOU (offer.owner (), flow.issuers.out, flow.issuers.out.issue ());

        if (result == tesSUCCESS)
            result = issueIOU (account (), flow.order.out, flow.order.out.issue ());
    }
    else
    {
        assert (isXRP (flow.order.out));

        if (result == tesSUCCESS)
            result = transferXRP (offer.owner (), account (), flow.order.out);
    }

    if (result == tesSUCCESS)
        direct_crossings_++;

    return result;
}

TER
Taker::fill (
    BasicTaker::Flow const& flow1, Offer& leg1,
    BasicTaker::Flow const& flow2, Offer& leg2)
{
    consume_offer (leg1, flow1.order);
    consume_offer (leg2, flow2.order);

    TER result = tesSUCCESS;

    if (leg1.owner () != account ())
    {
        if (result == tesSUCCESS)
            result = redeemIOU (account (), flow1.issuers.in, flow1.issuers.in.issue ());

        if (result == tesSUCCESS)
            result = issueIOU (leg1.owner (), flow1.order.in, flow1.order.in.issue ());
    }

    if (result == tesSUCCESS)
        result = transferXRP (leg1.owner (), leg2.owner (), flow1.order.out);

    if (leg2.owner () != account ())
    {
        if (result == tesSUCCESS)
            result = redeemIOU (leg2.owner (), flow2.issuers.out, flow2.issuers.out.issue ());

        if (result == tesSUCCESS)
            result = issueIOU (account (), flow2.order.out, flow2.order.out.issue ());
    }

    if (result == tesSUCCESS)
    {
        bridge_crossings_++;
        xrp_flow_ += flow1.order.out;
    }

    return result;
}

TER
Taker::cross (Offer& offer)
{
    if (isXRP (offer.amount ().in) && isXRP (offer.amount ().out))
        return tefINTERNAL;

    auto const amount = do_cross (
        offer.amount (), offer.quality (), offer.owner ());

    return fill (amount, offer);
}

TER
Taker::cross (Offer& leg1, Offer& leg2)
{
    if (isXRP (leg1.amount ().in) || isXRP (leg2.amount ().out))
        return tefINTERNAL;

    auto ret = do_cross (
        leg1.amount (), leg1.quality (), leg1.owner (),
        leg2.amount (), leg2.quality (), leg2.owner ());

    return fill (ret.first, leg1, ret.second, leg2);
}

Rate
Taker::calculateRate (
    ApplyView const& view,
        AccountID const& issuer,
            AccountID const& account)
{
    return isXRP (issuer) || (account == issuer)
        ? parityRate
        : transferRate (view, issuer);
}

} 

























