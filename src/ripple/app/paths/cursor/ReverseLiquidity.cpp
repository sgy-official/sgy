

#include <ripple/app/paths/cursor/PathCursor.h>
#include <ripple/basics/Log.h>
#include <ripple/ledger/View.h>
#include <tuple>

namespace ripple {
namespace path {


TER PathCursor::reverseLiquidity () const
{


    node().transferRate_ = transferRate (view(), node().issue_.account);

    if (node().isAccount ())
        return reverseLiquidityForAccount ();

    if (isXRP (nextNode().account_))
    {
        JLOG (j_.trace())
            << "reverseLiquidityForOffer: "
            << "OFFER --> offer: nodeIndex_=" << nodeIndex_;
        return tesSUCCESS;

    }

    STAmount saDeliverAct;

    JLOG (j_.trace())
        << "reverseLiquidityForOffer: OFFER --> account:"
        << " nodeIndex_=" << nodeIndex_
        << " saRevDeliver=" << node().saRevDeliver;

    return deliverNodeReverse (
        nextNode().account_,
        node().saRevDeliver,
        saDeliverAct);
}

} 
} 
























