

#include <ripple/app/paths/impl/AmountSpec.h>
#include <ripple/ledger/PaymentSandbox.h>
#include <ripple/ledger/View.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/SField.h>
#include <ripple/protocol/STAccount.h>
#include <boost/optional.hpp>

#include <cassert>

namespace ripple {

namespace detail {

auto DeferredCredits::makeKey (AccountID const& a1,
    AccountID const& a2,
    Currency const& c) -> Key
{
    if (a1 < a2)
        return std::make_tuple(a1, a2, c);
    else
        return std::make_tuple(a2, a1, c);
}

void
DeferredCredits::credit (AccountID const& sender,
    AccountID const& receiver,
    STAmount const& amount,
    STAmount const& preCreditSenderBalance)
{
    assert (sender != receiver);
    assert (!amount.negative());

    auto const k = makeKey (sender, receiver, amount.getCurrency ());
    auto i = credits_.find (k);
    if (i == credits_.end ())
    {
        Value v;

        if (sender < receiver)
        {
            v.highAcctCredits = amount;
            v.lowAcctCredits = amount.zeroed ();
            v.lowAcctOrigBalance = preCreditSenderBalance;
        }
        else
        {
            v.highAcctCredits = amount.zeroed ();
            v.lowAcctCredits = amount;
            v.lowAcctOrigBalance = -preCreditSenderBalance;
        }

        credits_[k] = v;
    }
    else
    {
        auto& v = i->second;
        if (sender < receiver)
            v.highAcctCredits += amount;
        else
            v.lowAcctCredits += amount;
    }
}

void
DeferredCredits::ownerCount (AccountID const& id,
    std::uint32_t cur,
    std::uint32_t next)
{
    auto const v = std::max (cur, next);
    auto r = ownerCounts_.emplace (std::make_pair (id, v));
    if (!r.second)
    {
        auto& mapVal = r.first->second;
        mapVal = std::max (v, mapVal);
    }
}

boost::optional<std::uint32_t>
DeferredCredits::ownerCount (AccountID const& id) const
{
    auto i = ownerCounts_.find (id);
    if (i != ownerCounts_.end ())
        return i->second;
    return boost::none;
}

auto
DeferredCredits::adjustments (AccountID const& main,
    AccountID const& other,
    Currency const& currency) const -> boost::optional<Adjustment>
{
    boost::optional<Adjustment> result;

    Key const k = makeKey (main, other, currency);
    auto i = credits_.find (k);
    if (i == credits_.end ())
        return result;

    auto const& v = i->second;

    if (main < other)
    {
        result.emplace (v.highAcctCredits, v.lowAcctCredits, v.lowAcctOrigBalance);
        return result;
    }
    else
    {
        result.emplace (v.lowAcctCredits, v.highAcctCredits, -v.lowAcctOrigBalance);
        return result;
    }
}

void DeferredCredits::apply(
    DeferredCredits& to)
{
    for (auto const& i : credits_)
    {
        auto r = to.credits_.emplace (i);
        if (!r.second)
        {
            auto& toVal = r.first->second;
            auto const& fromVal = i.second;
            toVal.lowAcctCredits += fromVal.lowAcctCredits;
            toVal.highAcctCredits += fromVal.highAcctCredits;
        }
    }

    for (auto const& i : ownerCounts_)
    {
        auto r = to.ownerCounts_.emplace (i);
        if (!r.second)
        {
            auto& toVal = r.first->second;
            auto const& fromVal = i.second;
            toVal = std::max (toVal, fromVal);
        }
    }
}

} 

STAmount
PaymentSandbox::balanceHook (AccountID const& account,
    AccountID const& issuer,
        STAmount const& amount) const
{
    

    auto const currency = amount.getCurrency ();
    auto const switchover = fix1141 (info ().parentCloseTime);

    auto adjustedAmt = amount;
    if (switchover)
    {
        auto delta = amount.zeroed ();
        auto lastBal = amount;
        auto minBal = amount;
        for (auto curSB = this; curSB; curSB = curSB->ps_)
        {
            if (auto adj = curSB->tab_.adjustments (account, issuer, currency))
            {
                delta += adj->debits;
                lastBal = adj->origBalance;
                if (lastBal < minBal)
                    minBal = lastBal;
            }
        }
        adjustedAmt = std::min(amount, lastBal - delta);
        if (rules().enabled(fix1368))
        {
            adjustedAmt = std::min(adjustedAmt, minBal);
        }
        if (fix1274 (info ().parentCloseTime))
            adjustedAmt.setIssuer(amount.getIssuer());
    }
    else
    {
        for (auto curSB = this; curSB; curSB = curSB->ps_)
        {
            if (auto adj = curSB->tab_.adjustments (account, issuer, currency))
            {
                adjustedAmt -= adj->credits;
            }
        }
    }

    if (isXRP(issuer) && adjustedAmt < beast::zero)
        adjustedAmt.clear();

    return adjustedAmt;
}

std::uint32_t
PaymentSandbox::ownerCountHook (AccountID const& account,
    std::uint32_t count) const
{
    std::uint32_t result = count;
    for (auto curSB = this; curSB; curSB = curSB->ps_)
    {
        if (auto adj = curSB->tab_.ownerCount (account))
            result = std::max (result, *adj);
    }
    return result;
}

void
PaymentSandbox::creditHook (AccountID const& from,
    AccountID const& to,
        STAmount const& amount,
            STAmount const& preCreditBalance)
{
    tab_.credit (from, to, amount, preCreditBalance);
}

void
PaymentSandbox::adjustOwnerCountHook (AccountID const& account,
    std::uint32_t cur,
        std::uint32_t next)
{
    tab_.ownerCount (account, cur, next);
}

void
PaymentSandbox::apply (RawView& to)
{
    assert(! ps_);
    items_.apply(to);
}

void
PaymentSandbox::apply (PaymentSandbox& to)
{
    assert(ps_ == &to);
    items_.apply(to);
    tab_.apply(to.tab_);
}

std::map<std::tuple<AccountID, AccountID, Currency>, STAmount>
PaymentSandbox::balanceChanges (ReadView const& view) const
{
    using key = std::tuple<AccountID, AccountID, Currency>;
    std::map<key, STAmount> result;

    auto each = [&result](uint256 const& key, bool isDelete,
        std::shared_ptr<SLE const> const& before,
        std::shared_ptr<SLE const> const& after) {

        STAmount oldBalance;
        STAmount newBalance;
        AccountID lowID;
        AccountID highID;

        if (isDelete)
        {
            if (!before)
                return;

            auto const bt = before->getType ();
            switch(bt)
            {
                case ltACCOUNT_ROOT:
                    lowID = xrpAccount();
                    highID = (*before)[sfAccount];
                    oldBalance = (*before)[sfBalance];
                    newBalance = oldBalance.zeroed();
                    break;
                case ltRIPPLE_STATE:
                    lowID = (*before)[sfLowLimit].getIssuer();
                    highID = (*before)[sfHighLimit].getIssuer();
                    oldBalance = (*before)[sfBalance];
                    newBalance = oldBalance.zeroed();
                    break;
                case ltOFFER:
                    break;
                default:
                    break;
            }
        }
        else if (!before)
        {
            auto const at = after->getType ();
            switch(at)
            {
                case ltACCOUNT_ROOT:
                    lowID = xrpAccount();
                    highID = (*after)[sfAccount];
                    newBalance = (*after)[sfBalance];
                    oldBalance = newBalance.zeroed();
                    break;
                case ltRIPPLE_STATE:
                    lowID = (*after)[sfLowLimit].getIssuer();
                    highID = (*after)[sfHighLimit].getIssuer();
                    newBalance = (*after)[sfBalance];
                    oldBalance = newBalance.zeroed();
                    break;
                case ltOFFER:
                    break;
                default:
                    break;
            }
        }
        else
        {
            auto const at = after->getType ();
            assert (at == before->getType ());
            switch(at)
            {
                case ltACCOUNT_ROOT:
                    lowID = xrpAccount();
                    highID = (*after)[sfAccount];
                    oldBalance = (*before)[sfBalance];
                    newBalance = (*after)[sfBalance];
                    break;
                case ltRIPPLE_STATE:
                    lowID = (*after)[sfLowLimit].getIssuer();
                    highID = (*after)[sfHighLimit].getIssuer();
                    oldBalance = (*before)[sfBalance];
                    newBalance = (*after)[sfBalance];
                    break;
                case ltOFFER:
                    break;
                default:
                    break;
            }
        }
        auto delta = newBalance - oldBalance;
        auto const cur = newBalance.getCurrency();
        result[std::make_tuple(lowID, highID, cur)] = delta;
        auto r = result.emplace(std::make_tuple(lowID, lowID, cur), delta);
        if (r.second)
        {
            r.first->second += delta;
        }

        delta.negate();
        r = result.emplace(std::make_tuple(highID, highID, cur), delta);
        if (r.second)
        {
            r.first->second += delta;
        }
    };
    items_.visit (view, each);
    return result;
}

XRPAmount PaymentSandbox::xrpDestroyed () const
{
    return items_.dropsDestroyed ();
}

}  
























