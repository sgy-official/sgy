

#include <ripple/app/paths/Credit.h>
#include <ripple/app/paths/cursor/RippleLiquidity.h>
#include <ripple/basics/Log.h>
#include <ripple/protocol/Quality.h>

namespace ripple {
namespace path {


TER PathCursor::reverseLiquidityForAccount () const
{
    TER terResult = tesSUCCESS;
    auto const lastNodeIndex = nodeSize () - 1;
    auto const isFinalNode = (nodeIndex_ == lastNodeIndex);

    std::uint64_t uRateMax = 0;

    const bool previousNodeIsAccount = !nodeIndex_ ||
            previousNode().isAccount();

    const bool nextNodeIsAccount = isFinalNode || nextNode().isAccount();

    AccountID const& previousAccountID = previousNodeIsAccount
        ? previousNode().account_ : node().account_;
    AccountID const& nextAccountID = nextNodeIsAccount ? nextNode().account_
        : node().account_;   

    auto const qualityIn
         = (nodeIndex_ != 0)
            ? quality_in (view(),
                node().account_,
                previousAccountID,
                node().issue_.currency)
            : parityRate;

    auto const qualityOut
        = (nodeIndex_ != lastNodeIndex)
            ? quality_out (view(),
                node().account_,
                nextAccountID,
                node().issue_.currency)
            : parityRate;

    const STAmount saPrvOwed = (previousNodeIsAccount && nodeIndex_ != 0)
        ? creditBalance (view(),
            node().account_,
            previousAccountID,
            node().issue_.currency)
        : STAmount (node().issue_);

    const STAmount saPrvLimit = (previousNodeIsAccount && nodeIndex_ != 0)
        ? creditLimit (view(),
            node().account_,
            previousAccountID,
            node().issue_.currency)
        : STAmount (node().issue_);

    const STAmount saNxtOwed = (nextNodeIsAccount && nodeIndex_ != lastNodeIndex)
        ? creditBalance (view(),
            node().account_,
            nextAccountID,
            node().issue_.currency)
        : STAmount (node().issue_);

    JLOG (j_.trace())
        << "reverseLiquidityForAccount>"
        << " nodeIndex_=" << nodeIndex_ << "/" << lastNodeIndex
        << " previousAccountID=" << previousAccountID
        << " node.account_=" << node().account_
        << " nextAccountID=" << nextAccountID
        << " currency=" << node().issue_.currency
        << " qualityIn=" << qualityIn
        << " qualityOut=" << qualityOut
        << " saPrvOwed=" << saPrvOwed
        << " saPrvLimit=" << saPrvLimit;

    const STAmount saPrvRedeemReq  = (saPrvOwed > beast::zero)
        ? saPrvOwed
        : STAmount (saPrvOwed.issue ());

    const STAmount saPrvIssueReq = (saPrvOwed < beast::zero)
        ? saPrvLimit + saPrvOwed : saPrvLimit;

    auto deliverCurrency = previousNode().saRevDeliver.getCurrency ();
    const STAmount saPrvDeliverReq (
        {deliverCurrency, previousNode().saRevDeliver.getIssuer ()}, -1);

    auto saCurRedeemAct = node().saRevRedeem.zeroed();

    auto saCurIssueAct = node().saRevIssue.zeroed();

    auto saCurDeliverAct  = node().saRevDeliver.zeroed();

    JLOG (j_.trace())
        << "reverseLiquidityForAccount:"
        << " saPrvRedeemReq:" << saPrvRedeemReq
        << " saPrvIssueReq:" << saPrvIssueReq
        << " previousNode.saRevDeliver:" << previousNode().saRevDeliver
        << " saPrvDeliverReq:" << saPrvDeliverReq
        << " node.saRevRedeem:" << node().saRevRedeem
        << " node.saRevIssue:" << node().saRevIssue
        << " saNxtOwed:" << saNxtOwed;


    assert (!node().saRevRedeem || -saNxtOwed >= node().saRevRedeem);
    assert (!node().saRevIssue  
            || saNxtOwed >= beast::zero
            || -saNxtOwed == node().saRevRedeem);

    if (nodeIndex_ == 0)
    {
    }

    else if (previousNodeIsAccount && nextNodeIsAccount)
    {
        if (isFinalNode)
        {
            const STAmount saCurWantedReq = std::min (
                pathState_.outReq() - pathState_.outAct(),
                saPrvLimit + saPrvOwed);
            auto saCurWantedAct = saCurWantedReq.zeroed ();

            JLOG (j_.trace())
                << "reverseLiquidityForAccount: account --> "
                << "ACCOUNT --> $ :"
                << " saCurWantedReq=" << saCurWantedReq;

            if (saPrvRedeemReq) 
            {

                saCurWantedAct = std::min (saPrvRedeemReq, saCurWantedReq);
                previousNode().saRevRedeem = saCurWantedAct;

                uRateMax = STAmount::uRateOne;

                JLOG (j_.trace())
                    << "reverseLiquidityForAccount: Redeem at 1:1"
                    << " saPrvRedeemReq=" << saPrvRedeemReq
                    << " (available) previousNode.saRevRedeem="
                    << previousNode().saRevRedeem
                    << " uRateMax="
                    << amountFromQuality (uRateMax).getText ();
            }
            else
            {
                previousNode().saRevRedeem.clear (saPrvRedeemReq);
            }

            previousNode().saRevIssue.clear (saPrvIssueReq);

            if (saCurWantedReq != saCurWantedAct 
                && saPrvIssueReq)  
            {

                rippleLiquidity (
                    rippleCalc_,
                    qualityIn,
                    parityRate,
                    saPrvIssueReq,
                    saCurWantedReq,
                    previousNode().saRevIssue,
                    saCurWantedAct,
                    uRateMax);

                JLOG (j_.trace())
                    << "reverseLiquidityForAccount: Issuing: Rate: "
                    << "quality in : 1.0"
                    << " previousNode.saRevIssue:" << previousNode().saRevIssue
                    << " saCurWantedAct:" << saCurWantedAct;
            }

            if (!saCurWantedAct)
            {
                terResult   = tecPATH_DRY;
            }
        }
        else
        {
            previousNode().saRevRedeem.clear (saPrvRedeemReq);
            previousNode().saRevIssue.clear (saPrvIssueReq);

            if (node().saRevRedeem
                && saPrvRedeemReq)
            {
                rippleLiquidity (
                    rippleCalc_,
                    parityRate,
                    qualityOut,
                    saPrvRedeemReq,
                    node().saRevRedeem,
                    previousNode().saRevRedeem,
                    saCurRedeemAct,
                    uRateMax);

                JLOG (j_.trace())
                    << "reverseLiquidityForAccount: "
                    << "Rate : 1.0 : quality out"
                    << " previousNode.saRevRedeem:" << previousNode().saRevRedeem
                    << " saCurRedeemAct:" << saCurRedeemAct;
            }

            if (node().saRevRedeem != saCurRedeemAct
                && previousNode().saRevRedeem == saPrvRedeemReq)
            {
                rippleLiquidity (
                    rippleCalc_,
                    qualityIn,
                    qualityOut,
                    saPrvIssueReq,
                    node().saRevRedeem,
                    previousNode().saRevIssue,
                    saCurRedeemAct,
                    uRateMax);

                JLOG (j_.trace())
                    << "reverseLiquidityForAccount: "
                    << "Rate: quality in : quality out:"
                    << " previousNode.saRevIssue:" << previousNode().saRevIssue
                    << " saCurRedeemAct:" << saCurRedeemAct;
            }

            if (node().saRevIssue   
                && saCurRedeemAct == node().saRevRedeem
                && previousNode().saRevRedeem != saPrvRedeemReq)
            {
                rippleLiquidity (
                    rippleCalc_,
                    parityRate,
                    transferRate (view(), node().account_),
                    saPrvRedeemReq,
                    node().saRevIssue,
                    previousNode().saRevRedeem,
                    saCurIssueAct,
                    uRateMax);

                JLOG (j_.debug())
                    << "reverseLiquidityForAccount: "
                    << "Rate : 1.0 : transfer_rate:"
                    << " previousNode.saRevRedeem:" << previousNode().saRevRedeem
                    << " saCurIssueAct:" << saCurIssueAct;
            }

            if (node().saRevIssue != saCurIssueAct
                && saCurRedeemAct == node().saRevRedeem
                && saPrvRedeemReq == previousNode().saRevRedeem
                && saPrvIssueReq)
            {
                rippleLiquidity (
                    rippleCalc_,
                    qualityIn,
                    parityRate,
                    saPrvIssueReq,
                    node().saRevIssue,
                    previousNode().saRevIssue,
                    saCurIssueAct,
                    uRateMax);

                JLOG (j_.trace())
                    << "reverseLiquidityForAccount: "
                    << "Rate: quality in : 1.0:"
                    << " previousNode.saRevIssue:" << previousNode().saRevIssue
                    << " saCurIssueAct:" << saCurIssueAct;
            }

            if (!saCurRedeemAct && !saCurIssueAct)
            {
                terResult = tecPATH_DRY;
            }

            JLOG (j_.trace())
                << "reverseLiquidityForAccount: "
                << "^|account --> ACCOUNT --> account :"
                << " node.saRevRedeem:" << node().saRevRedeem
                << " node.saRevIssue:" << node().saRevIssue
                << " saPrvOwed:" << saPrvOwed
                << " saCurRedeemAct:" << saCurRedeemAct
                << " saCurIssueAct:" << saCurIssueAct;
        }
    }
    else if (previousNodeIsAccount && !nextNodeIsAccount)
    {
        JLOG (j_.trace())
            << "reverseLiquidityForAccount: "
            << "account --> ACCOUNT --> offer";

        previousNode().saRevRedeem.clear (saPrvRedeemReq);
        previousNode().saRevIssue.clear (saPrvIssueReq);

        if (saPrvOwed > beast::zero                    
            && node().saRevDeliver)                 
        {
            rippleLiquidity (
                rippleCalc_,
                parityRate,
                transferRate (view(), node().account_),
                saPrvRedeemReq,
                node().saRevDeliver,
                previousNode().saRevRedeem,
                saCurDeliverAct,
                uRateMax);
        }

        if (saPrvRedeemReq == previousNode().saRevRedeem
            && node().saRevDeliver != saCurDeliverAct)  
        {
            rippleLiquidity (
                rippleCalc_,
                qualityIn,
                parityRate,
                saPrvIssueReq,
                node().saRevDeliver,
                previousNode().saRevIssue,
                saCurDeliverAct,
                uRateMax);
        }

        if (!saCurDeliverAct)
        {
            terResult   = tecPATH_DRY;
        }

        JLOG (j_.trace())
            << "reverseLiquidityForAccount: "
            << " node.saRevDeliver:" << node().saRevDeliver
            << " saCurDeliverAct:" << saCurDeliverAct
            << " saPrvOwed:" << saPrvOwed;
    }
    else if (!previousNodeIsAccount && nextNodeIsAccount)
    {
        if (isFinalNode)
        {
            STAmount const& saCurWantedReq  =
                    pathState_.outReq() - pathState_.outAct();
            STAmount saCurWantedAct = saCurWantedReq.zeroed();

            JLOG (j_.trace())
                << "reverseLiquidityForAccount: "
                << "offer --> ACCOUNT --> $ :"
                << " saCurWantedReq:" << saCurWantedReq
                << " saOutAct:" << pathState_.outAct()
                << " saOutReq:" << pathState_.outReq();

            if (saCurWantedReq <= beast::zero)
            {
                assert(false);
                JLOG (j_.fatal()) << "CurWantReq was not positive";
                return tefEXCEPTION;
            }



            rippleLiquidity (
                rippleCalc_,
                qualityIn,
                parityRate,
                saPrvDeliverReq,
                saCurWantedReq,
                previousNode().saRevDeliver,
                saCurWantedAct,
                uRateMax);

            if (!saCurWantedAct)
            {
                terResult   = tecPATH_DRY;
            }

            JLOG (j_.trace())
                << "reverseLiquidityForAccount:"
                << " previousNode().saRevDeliver:" << previousNode().saRevDeliver
                << " saPrvDeliverReq:" << saPrvDeliverReq
                << " saCurWantedAct:" << saCurWantedAct
                << " saCurWantedReq:" << saCurWantedReq;
        }
        else
        {
            JLOG (j_.trace())
                << "reverseLiquidityForAccount: "
                << "offer --> ACCOUNT --> account :"
                << " node.saRevRedeem:" << node().saRevRedeem
                << " node.saRevIssue:" << node().saRevIssue;

            if (node().saRevRedeem)  
            {
                rippleLiquidity (
                    rippleCalc_,
                    parityRate,
                    qualityOut,
                    saPrvDeliverReq,
                    node().saRevRedeem,
                    previousNode().saRevDeliver,
                    saCurRedeemAct,
                    uRateMax);
            }

            if (node().saRevRedeem == saCurRedeemAct
                && node().saRevIssue)
            {
                rippleLiquidity (
                    rippleCalc_,
                    parityRate,
                    transferRate (view(), node().account_),
                    saPrvDeliverReq,
                    node().saRevIssue,
                    previousNode().saRevDeliver,
                    saCurIssueAct,
                    uRateMax);
            }

            JLOG (j_.trace())
                << "reverseLiquidityForAccount:"
                << " saCurRedeemAct:" << saCurRedeemAct
                << " node.saRevRedeem:" << node().saRevRedeem
                << " previousNode.saRevDeliver:" << previousNode().saRevDeliver
                << " node.saRevIssue:" << node().saRevIssue;

            if (!previousNode().saRevDeliver)
            {
                terResult   = tecPATH_DRY;
            }
        }
    }
    else
    {
        JLOG (j_.trace())
            << "reverseLiquidityForAccount: offer --> ACCOUNT --> offer";

        rippleLiquidity (
            rippleCalc_,
            parityRate,
            transferRate (view(), node().account_),
            saPrvDeliverReq,
            node().saRevDeliver,
            previousNode().saRevDeliver,
            saCurDeliverAct,
            uRateMax);

        if (!saCurDeliverAct)
        {
            terResult   = tecPATH_DRY;
        }
    }

    return terResult;
}

} 
} 
























