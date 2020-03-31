

#include <ripple/app/paths/cursor/RippleLiquidity.h>
#include <ripple/basics/Log.h>
#include <ripple/protocol/Quality.h>

namespace ripple {
namespace path {

TER PathCursor::forwardLiquidityForAccount () const
{
    TER resultCode   = tesSUCCESS;
    auto const lastNodeIndex       = pathState_.nodes().size () - 1;
    auto viewJ = rippleCalc_.logs_.journal ("View");

    std::uint64_t uRateMax = 0;

    AccountID const& previousAccountID =
            previousNode().isAccount() ? previousNode().account_ :
            node().account_;
    AccountID const& nextAccountID =
            nextNode().isAccount() ? nextNode().account_ : node().account_;

    auto const qualityIn = nodeIndex_
        ? quality_in (view(),
            node().account_,
            previousAccountID,
            node().issue_.currency)
        : parityRate;

    auto const qualityOut = (nodeIndex_ == lastNodeIndex)
        ? quality_out (view(),
            node().account_,
            nextAccountID,
            node().issue_.currency)
        : parityRate;


    auto saPrvRedeemAct = previousNode().saFwdRedeem.zeroed();
    auto saPrvIssueAct = previousNode().saFwdIssue.zeroed();

    auto saPrvDeliverAct = previousNode().saFwdDeliver.zeroed ();

    JLOG (j_.trace())
        << "forwardLiquidityForAccount> "
        << "nodeIndex_=" << nodeIndex_ << "/" << lastNodeIndex
        << " previousNode.saFwdRedeem:" << previousNode().saFwdRedeem
        << " saPrvIssueReq:" << previousNode().saFwdIssue
        << " previousNode.saFwdDeliver:" << previousNode().saFwdDeliver
        << " node.saRevRedeem:" << node().saRevRedeem
        << " node.saRevIssue:" << node().saRevIssue
        << " node.saRevDeliver:" << node().saRevDeliver;


    if (previousNode().isAccount() && nextNode().isAccount())
    {

        if (!nodeIndex_)
        {

            node().saFwdRedeem = node().saRevRedeem;

            if (pathState_.inReq() >= beast::zero)
            {
                node().saFwdRedeem = std::min (
                    node().saFwdRedeem, pathState_.inReq() - pathState_.inAct());
            }

            pathState_.setInPass (node().saFwdRedeem);

            node().saFwdIssue = node().saFwdRedeem == node().saRevRedeem
                ? node().saRevIssue : STAmount (node().saRevIssue);

            if (node().saFwdIssue && pathState_.inReq() >= beast::zero)
            {
                node().saFwdIssue = std::min (
                    node().saFwdIssue,
                    pathState_.inReq() - pathState_.inAct() - node().saFwdRedeem);
            }

            pathState_.setInPass (pathState_.inPass() + node().saFwdIssue);

            JLOG (j_.trace())
                << "forwardLiquidityForAccount: ^ --> "
                << "ACCOUNT --> account :"
                << " saInReq=" << pathState_.inReq()
                << " saInAct=" << pathState_.inAct()
                << " node.saFwdRedeem:" << node().saFwdRedeem
                << " node.saRevIssue:" << node().saRevIssue
                << " node.saFwdIssue:" << node().saFwdIssue
                << " pathState_.saInPass:" << pathState_.inPass();
        }
        else if (nodeIndex_ == lastNodeIndex)
        {
            JLOG (j_.trace())
                << "forwardLiquidityForAccount: account --> "
                << "ACCOUNT --> $ :"
                << " previousAccountID="
                << to_string (previousAccountID)
                << " node.account_="
                << to_string (node().account_)
                << " previousNode.saFwdRedeem:" << previousNode().saFwdRedeem
                << " previousNode.saFwdIssue:" << previousNode().saFwdIssue;


            auto& saCurReceive = pathState_.outPass();
            STAmount saIssueCrd = qualityIn >= parityRate
                    ? previousNode().saFwdIssue  
                    : multiplyRound (
                          previousNode().saFwdIssue,
                          qualityIn,
                          true); 

            pathState_.setOutPass (previousNode().saFwdRedeem + saIssueCrd);

            if (saCurReceive)
            {
                resultCode = rippleCredit(view(),
                    previousAccountID,
                    node().account_,
                    previousNode().saFwdRedeem + previousNode().saFwdIssue,
                    false, viewJ);
            }
            else
            {
                resultCode   = tecPATH_DRY;
            }
        }
        else
        {
            JLOG (j_.trace())
                << "forwardLiquidityForAccount: account --> "
                << "ACCOUNT --> account";

            node().saFwdRedeem.clear (node().saRevRedeem);
            node().saFwdIssue.clear (node().saRevIssue);

            if (previousNode().saFwdRedeem && node().saRevRedeem)
            {
                rippleLiquidity (
                    rippleCalc_,
                    parityRate,
                    qualityOut,
                    previousNode().saFwdRedeem,
                    node().saRevRedeem,
                    saPrvRedeemAct,
                    node().saFwdRedeem,
                    uRateMax);
            }

            if (previousNode().saFwdIssue != saPrvIssueAct
                && node().saRevRedeem != node().saFwdRedeem)
            {
                rippleLiquidity (
                    rippleCalc_,
                    qualityIn,
                    qualityOut,
                    previousNode().saFwdIssue,
                    node().saRevRedeem,
                    saPrvIssueAct,
                    node().saFwdRedeem,
                    uRateMax);
            }

            if (previousNode().saFwdRedeem != saPrvRedeemAct
                && node().saRevRedeem == node().saFwdRedeem
                && node().saRevIssue)
            {
                rippleLiquidity (
                    rippleCalc_,
                    parityRate,
                    transferRate (view(), node().account_),
                    previousNode().saFwdRedeem,
                    node().saRevIssue,
                    saPrvRedeemAct,
                    node().saFwdIssue,
                    uRateMax);
            }

            if (previousNode().saFwdIssue != saPrvIssueAct
                && node().saRevRedeem == node().saFwdRedeem
                && node().saRevIssue)
            {
                rippleLiquidity (
                    rippleCalc_,
                    qualityIn,
                    parityRate,
                    previousNode().saFwdIssue,
                    node().saRevIssue,
                    saPrvIssueAct,
                    node().saFwdIssue,
                    uRateMax);
            }

            STAmount saProvide = node().saFwdRedeem + node().saFwdIssue;

            resultCode = saProvide
                ? rippleCredit(view(),
                    previousAccountID,
                    node().account_,
                    previousNode().saFwdRedeem + previousNode().saFwdIssue,
                    false, viewJ)
                : tecPATH_DRY;
        }
    }
    else if (previousNode().isAccount() && !nextNode().isAccount())
    {

        if (nodeIndex_)
        {
            JLOG (j_.trace())
                << "forwardLiquidityForAccount: account --> "
                << "ACCOUNT --> offer";

            node().saFwdDeliver.clear (node().saRevDeliver);

            if (previousNode().saFwdRedeem)
            {
                rippleLiquidity (
                    rippleCalc_,
                    parityRate,
                    transferRate (view(), node().account_),
                    previousNode().saFwdRedeem,
                    node().saRevDeliver,
                    saPrvRedeemAct,
                    node().saFwdDeliver,
                    uRateMax);
            }

            if (previousNode().saFwdRedeem == saPrvRedeemAct
                && previousNode().saFwdIssue)
            {
                rippleLiquidity (
                    rippleCalc_,
                    qualityIn,
                    parityRate,
                    previousNode().saFwdIssue,
                    node().saRevDeliver,
                    saPrvIssueAct,
                    node().saFwdDeliver,
                    uRateMax);
            }

            resultCode   = node().saFwdDeliver
                ? rippleCredit(view(),
                    previousAccountID, node().account_,
                    previousNode().saFwdRedeem + previousNode().saFwdIssue,
                    false, viewJ)
                : tecPATH_DRY;  
        }
        else
        {
            node().saFwdDeliver = node().saRevDeliver;

            if (pathState_.inReq() >= beast::zero)
            {
                node().saFwdDeliver = std::min (
                    node().saFwdDeliver, pathState_.inReq() - pathState_.inAct());

                if (isXRP (node().issue_))
                    node().saFwdDeliver = std::min (
                        node().saFwdDeliver,
                        accountHolds(view(),
                            node().account_,
                            xrpCurrency(),
                            xrpAccount(),
                            fhIGNORE_FREEZE, viewJ)); 

            }

            pathState_.setInPass (node().saFwdDeliver);

            if (!node().saFwdDeliver)
            {
                resultCode   = tecPATH_DRY;
            }
            else if (!isXRP (node().issue_))
            {

                JLOG (j_.trace())
                    << "forwardLiquidityForAccount: ^ --> "
                    << "ACCOUNT -- !XRP --> offer";

            }
            else
            {
                JLOG (j_.trace())
                    << "forwardLiquidityForAccount: ^ --> "
                    << "ACCOUNT -- XRP --> offer";

                resultCode = accountSend(view(),
                    node().account_, xrpAccount(), node().saFwdDeliver, viewJ);
            }
        }
    }
    else if (!previousNode().isAccount() && nextNode().isAccount())
    {
        if (nodeIndex_ == lastNodeIndex)
        {
            JLOG (j_.trace())
                << "forwardLiquidityForAccount: offer --> "
                << "ACCOUNT --> $ : "
                << previousNode().saFwdDeliver;

            pathState_.setOutPass (previousNode().saFwdDeliver);

        }
        else
        {
            JLOG (j_.trace())
                << "forwardLiquidityForAccount: offer --> "
                << "ACCOUNT --> account";

            node().saFwdRedeem.clear (node().saRevRedeem);
            node().saFwdIssue.clear (node().saRevIssue);

            if (previousNode().saFwdDeliver && node().saRevRedeem)
            {
                rippleLiquidity (
                    rippleCalc_,
                    parityRate,
                    qualityOut,
                    previousNode().saFwdDeliver,
                    node().saRevRedeem,
                    saPrvDeliverAct,
                    node().saFwdRedeem,
                    uRateMax);
            }

            if (previousNode().saFwdDeliver != saPrvDeliverAct
                && node().saRevRedeem == node().saFwdRedeem
                && node().saRevIssue)
            {
                rippleLiquidity (
                    rippleCalc_,
                    parityRate,
                    transferRate (view(), node().account_),
                    previousNode().saFwdDeliver,
                    node().saRevIssue,
                    saPrvDeliverAct,
                    node().saFwdIssue,
                    uRateMax);
            }

            STAmount saProvide = node().saFwdRedeem + node().saFwdIssue;

            if (!saProvide)
                resultCode = tecPATH_DRY;
        }
    }
    else
    {
        JLOG (j_.trace())
            << "forwardLiquidityForAccount: offer --> ACCOUNT --> offer";

        node().saFwdDeliver.clear (node().saRevDeliver);

        if (previousNode().saFwdDeliver && node().saRevDeliver)
        {
            rippleLiquidity (
                rippleCalc_,
                parityRate,
                transferRate (view(), node().account_),
                previousNode().saFwdDeliver,
                node().saRevDeliver,
                saPrvDeliverAct,
                node().saFwdDeliver,
                uRateMax);
        }

        if (!node().saFwdDeliver)
            resultCode   = tecPATH_DRY;
    }

    return resultCode;
}

} 
} 
























