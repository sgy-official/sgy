

#include <ripple/app/paths/cursor/PathCursor.h>
#include <ripple/basics/Log.h>
#include <tuple>

namespace ripple {
namespace path {

TER PathCursor::forwardLiquidity () const
{
    if (node().isAccount())
        return forwardLiquidityForAccount ();

    if (previousNode().account_ == beast::zero)
        return tesSUCCESS;

    STAmount saInAct;
    STAmount saInFees;

    auto resultCode = deliverNodeForward (
        previousNode().account_,
        previousNode().saFwdDeliver, 
        saInAct,
        saInFees,
        false);

    assert (resultCode != tesSUCCESS ||
            previousNode().saFwdDeliver == saInAct + saInFees);
    return resultCode;
}

} 
} 


























