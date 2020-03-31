

#include <ripple/ledger/ReadView.h>
#include <ripple/protocol/AmountConversions.h>
#include <ripple/protocol/STAmount.h>
#include <ripple/protocol/Indexes.h>

namespace ripple {

STAmount
creditLimit (
    ReadView const& view,
    AccountID const& account,
    AccountID const& issuer,
    Currency const& currency)
{
    STAmount result ({currency, account});

    auto sleRippleState = view.read(
        keylet::line(account, issuer, currency));

    if (sleRippleState)
    {
        result = sleRippleState->getFieldAmount (
            account < issuer ? sfLowLimit : sfHighLimit);
        result.setIssuer (account);
    }

    assert (result.getIssuer () == account);
    assert (result.getCurrency () == currency);
    return result;
}

IOUAmount
creditLimit2 (
    ReadView const& v,
    AccountID const& acc,
    AccountID const& iss,
    Currency const& cur)
{
    return toAmount<IOUAmount> (creditLimit (v, acc, iss, cur));
}

STAmount creditBalance (
    ReadView const& view,
    AccountID const& account,
    AccountID const& issuer,
    Currency const& currency)
{
    STAmount result ({currency, account});

    auto sleRippleState = view.read(
        keylet::line(account, issuer, currency));

    if (sleRippleState)
    {
        result = sleRippleState->getFieldAmount (sfBalance);
        if (account < issuer)
            result.negate ();
        result.setIssuer (account);
    }

    assert (result.getIssuer () == account);
    assert (result.getCurrency () == currency);
    return result;
}

} 
























