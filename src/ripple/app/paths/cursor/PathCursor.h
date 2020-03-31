

#ifndef RIPPLE_APP_PATHS_CURSOR_PATHCURSOR_H_INCLUDED
#define RIPPLE_APP_PATHS_CURSOR_PATHCURSOR_H_INCLUDED

#include <ripple/app/paths/RippleCalc.h>

namespace ripple {
namespace path {


class PathCursor
{
public:
    PathCursor(
        RippleCalc& rippleCalc,
        PathState& pathState,
        bool multiQuality,
        beast::Journal j,
        NodeIndex nodeIndex = 0)
            : rippleCalc_(rippleCalc),
              pathState_(pathState),
              multiQuality_(multiQuality),
              nodeIndex_(restrict(nodeIndex)),
              j_ (j)
    {
    }

    void nextIncrement() const;

private:
    PathCursor(PathCursor const&) = default;

    PathCursor increment(int delta = 1) const
    {
        return {rippleCalc_, pathState_, multiQuality_, j_, nodeIndex_ + delta};
    }

    TER liquidity() const;
    TER reverseLiquidity () const;
    TER forwardLiquidity () const;

    TER forwardLiquidityForAccount () const;
    TER reverseLiquidityForOffer () const;
    TER forwardLiquidityForOffer () const;
    TER reverseLiquidityForAccount () const;

    
    TER advanceNode (bool reverse) const;
    TER advanceNode (STAmount const& amount, bool reverse, bool callerHasLiquidity) const;

    TER deliverNodeReverse (
        AccountID const& uOutAccountID,
        STAmount const& saOutReq,
        STAmount& saOutAct) const;

    TER deliverNodeReverseImpl (
        AccountID const& uOutAccountID,
        STAmount const& saOutReq,
        STAmount& saOutAct,
        bool callerHasLiquidity) const;

    TER deliverNodeForward (
        AccountID const& uInAccountID,
        STAmount const& saInReq,
        STAmount& saInAct,
        STAmount& saInFees,
        bool callerHasLiquidity) const;

    PaymentSandbox&
    view() const
    {
        return pathState_.view();
    }

    NodeIndex nodeSize() const
    {
        return pathState_.nodes().size();
    }

    NodeIndex restrict(NodeIndex i) const
    {
        return std::min (i, nodeSize() - 1);
    }

    Node& node(NodeIndex i) const
    {
        return pathState_.nodes()[i];
    }

    Node& node() const
    {
        return node (nodeIndex_);
    }

    Node& previousNode() const
    {
        return node (restrict (nodeIndex_ - 1));
    }

    Node& nextNode() const
    {
        return node (restrict (nodeIndex_ + 1));
    }

    RippleCalc& rippleCalc_;
    PathState& pathState_;
    bool multiQuality_;
    NodeIndex nodeIndex_;
    beast::Journal j_;
};

} 
} 

#endif








