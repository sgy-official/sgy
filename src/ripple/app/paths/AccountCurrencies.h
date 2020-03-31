

#ifndef RIPPLE_APP_PATHS_ACCOUNTCURRENCIES_H_INCLUDED
#define RIPPLE_APP_PATHS_ACCOUNTCURRENCIES_H_INCLUDED

#include <ripple/app/paths/RippleLineCache.h>
#include <ripple/protocol/UintTypes.h>

namespace ripple {

hash_set<Currency>
accountDestCurrencies(
    AccountID const& account,
        std::shared_ptr<RippleLineCache> const& cache,
            bool includeXRP);

hash_set<Currency>
accountSourceCurrencies(
    AccountID const& account,
        std::shared_ptr<RippleLineCache> const& lrLedger,
             bool includeXRP);

} 

#endif








