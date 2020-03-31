

#include <ripple/app/paths/cursor/EffectiveRate.h>
#include <ripple/basics/contract.h>

namespace ripple {
namespace path {

Rate
effectiveRate(
    Issue const& issue,
    AccountID const& account1,
    AccountID const& account2,
    boost::optional<Rate> const& rate)
{
    if (isXRP (issue))
        return parityRate;

    if (!rate)
        LogicError ("No transfer rate set for node.");

    if (issue.account == account1 || issue.account == account2)
        return parityRate;

    return rate.get();
}

} 
} 
























