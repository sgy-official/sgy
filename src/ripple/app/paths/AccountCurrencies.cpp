

#include <ripple/app/paths/AccountCurrencies.h>

namespace ripple {

hash_set<Currency> accountSourceCurrencies (
    AccountID const& account,
    std::shared_ptr<RippleLineCache> const& lrCache,
    bool includeXRP)
{
    hash_set<Currency> currencies;

    if (includeXRP)
        currencies.insert (xrpCurrency());

    auto& rippleLines = lrCache->getRippleLines (account);

    for (auto const& item : rippleLines)
    {
        auto rspEntry = (RippleState*) item.get ();
        assert (rspEntry);
        if (!rspEntry)
            continue;

        auto& saBalance = rspEntry->getBalance ();

        if (saBalance > beast::zero
            || (rspEntry->getLimitPeer ()
                && ((-saBalance) < rspEntry->getLimitPeer ()))) 
        {
            currencies.insert (saBalance.getCurrency ());
        }
    }

    currencies.erase (badCurrency());
    return currencies;
}

hash_set<Currency> accountDestCurrencies (
    AccountID const& account,
    std::shared_ptr<RippleLineCache> const& lrCache,
    bool includeXRP)
{
    hash_set<Currency> currencies;

    if (includeXRP)
        currencies.insert (xrpCurrency());

    auto& rippleLines = lrCache->getRippleLines (account);

    for (auto const& item : rippleLines)
    {
        auto rspEntry = (RippleState*) item.get ();
        assert (rspEntry);
        if (!rspEntry)
            continue;

        auto& saBalance  = rspEntry->getBalance ();

        if (saBalance < rspEntry->getLimit ())                  
            currencies.insert (saBalance.getCurrency ());
    }

    currencies.erase (badCurrency());
    return currencies;
}

} 
























