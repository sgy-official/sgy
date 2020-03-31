

#include <ripple/app/paths/cursor/EffectiveRate.h>
#include <ripple/app/paths/cursor/RippleLiquidity.h>
#include <ripple/basics/Log.h>

namespace ripple {
namespace path {


TER PathCursor::deliverNodeForward (
    AccountID const& uInAccountID,    
    STAmount const& saInReq,        
    STAmount& saInAct,              
    STAmount& saInFees,             
    bool callerHasLiquidity) const
{
    TER resultCode   = tesSUCCESS;

    node().directory.restart(multiQuality_);

    saInAct.clear (saInReq);
    saInFees.clear (saInReq);

    int loopCount = 0;
    auto viewJ = rippleCalc_.logs_.journal ("View");

    while (resultCode == tesSUCCESS && saInAct + saInFees < saInReq)
    {
        if (++loopCount >
            (multiQuality_ ?
                CALC_NODE_DELIVER_MAX_LOOPS_MQ :
                CALC_NODE_DELIVER_MAX_LOOPS))
        {
            JLOG (j_.warn())
                << "deliverNodeForward: max loops cndf";
            return telFAILED_PROCESSING;
        }

        advanceNode (saInAct, false, callerHasLiquidity);


        if (resultCode != tesSUCCESS)
        {
        }
        else if (!node().offerIndex_)
        {
            JLOG (j_.warn())
                << "deliverNodeForward: INTERNAL ERROR: Ran out of offers.";
            return telFAILED_PROCESSING;
        }
        else if (resultCode == tesSUCCESS)
        {
            auto const xferRate = effectiveRate (
                previousNode().issue_,
                uInAccountID,
                node().offerOwnerAccount_,
                previousNode().transferRate_);


            auto saOutFunded = std::min (
                node().saOfferFunds, node().saTakerGets);

            auto saOutPassFunded = std::min (
                saOutFunded,
                node().saRevDeliver - node().saFwdDeliver);

            auto saInFunded = mulRound (
                saOutPassFunded,
                node().saOfrRate,
                node().saTakerPays.issue (),
                true);

            auto saInTotal = multiplyRound (
                saInFunded, xferRate, true);
            auto saInRemaining = saInReq - saInAct - saInFees;

            if (saInRemaining < beast::zero)
                saInRemaining.clear();

            auto saInSum = std::min (saInTotal, saInRemaining);

            auto saInPassAct = std::min (
                node().saTakerPays,
                divideRound (saInSum, xferRate, true));

            auto outPass = divRound (
                saInPassAct, node().saOfrRate, node().saTakerGets.issue (), true);
            STAmount saOutPassMax    = std::min (saOutPassFunded, outPass);

            STAmount saInPassFeesMax = saInSum - saInPassAct;

            STAmount saOutPassAct;

            STAmount saInPassFees;

            JLOG (j_.trace())
                << "deliverNodeForward:"
                << " nodeIndex_=" << nodeIndex_
                << " saOutFunded=" << saOutFunded
                << " saOutPassFunded=" << saOutPassFunded
                << " node().saOfferFunds=" << node().saOfferFunds
                << " node().saTakerGets=" << node().saTakerGets
                << " saInReq=" << saInReq
                << " saInAct=" << saInAct
                << " saInFees=" << saInFees
                << " saInFunded=" << saInFunded
                << " saInTotal=" << saInTotal
                << " saInSum=" << saInSum
                << " saInPassAct=" << saInPassAct
                << " saOutPassMax=" << saOutPassMax;

            if (!node().saTakerPays || saInSum <= beast::zero)
            {
                JLOG (j_.debug())
                    << "deliverNodeForward: Microscopic offer unfunded.";

                pathState_.unfundedOffers().push_back (node().offerIndex_);
                node().bEntryAdvance = true;
                continue;
            }

            if (!saInFunded)
            {
                JLOG (j_.warn())
                    << "deliverNodeForward: UNREACHABLE REACHED";

                pathState_.unfundedOffers().push_back (node().offerIndex_);
                node().bEntryAdvance = true;
                continue;
            }

            if (!isXRP(nextNode().account_))
            {

                saOutPassAct = saOutPassMax;
                saInPassFees = saInPassFeesMax;

                JLOG (j_.trace())
                    << "deliverNodeForward: ? --> OFFER --> account:"
                    << " offerOwnerAccount_="
                    << node().offerOwnerAccount_
                    << " nextNode().account_="
                    << nextNode().account_
                    << " saOutPassAct=" << saOutPassAct
                    << " saOutFunded=" << saOutFunded;

                resultCode = accountSend(view(),
                    node().offerOwnerAccount_,
                    nextNode().account_,
                    saOutPassAct, viewJ);

                if (resultCode != tesSUCCESS)
                    break;
            }
            else
            {
                STAmount saOutPassFees;

                resultCode = increment().deliverNodeForward (
                    node().offerOwnerAccount_,  
                    saOutPassMax,             
                    saOutPassAct,             
                    saOutPassFees,            
                    saInAct > beast::zero);

                if (resultCode != tesSUCCESS)
                    break;

                if (saOutPassAct == saOutPassMax)
                {

                    saInPassFees = saInPassFeesMax;
                }
                else
                {

                    assert (saOutPassAct < saOutPassMax);
                    auto inPassAct = mulRound (
                        saOutPassAct, node().saOfrRate, saInReq.issue (), true);
                    saInPassAct = std::min (node().saTakerPays, inPassAct);
                    auto inPassFees = multiplyRound (
                        saInPassAct, xferRate, true);
                    saInPassFees    = std::min (saInPassFeesMax, inPassFees);
                }

                auto const& id = isXRP(node().issue_) ?
                        xrpAccount() : node().issue_.account;
                auto outPassTotal = saOutPassAct + saOutPassFees;
                accountSend(view(),
                    node().offerOwnerAccount_,
                    id,
                    outPassTotal,
                    viewJ);

                JLOG (j_.trace())
                    << "deliverNodeForward: ? --> OFFER --> offer:"
                    << " saOutPassAct=" << saOutPassAct
                    << " saOutPassFees=" << saOutPassFees;
            }

            JLOG (j_.trace())
                << "deliverNodeForward: "
                << " nodeIndex_=" << nodeIndex_
                << " node().saTakerGets=" << node().saTakerGets
                << " node().saTakerPays=" << node().saTakerPays
                << " saInPassAct=" << saInPassAct
                << " saInPassFees=" << saInPassFees
                << " saOutPassAct=" << saOutPassAct
                << " saOutFunded=" << saOutFunded;

            node().bFundsDirty = true;

            if (isXRP (previousNode().issue_.currency)
                || uInAccountID != node().offerOwnerAccount_)
            {
                auto id = !isXRP(previousNode().issue_.currency) ?
                        uInAccountID : xrpAccount();
                resultCode = accountSend(view(),
                    id,
                    node().offerOwnerAccount_,
                    saInPassAct,
                    viewJ);

                if (resultCode != tesSUCCESS)
                    break;
            }

            STAmount saTakerGetsNew  = node().saTakerGets - saOutPassAct;
            STAmount saTakerPaysNew  = node().saTakerPays - saInPassAct;

            if (saTakerPaysNew < beast::zero || saTakerGetsNew < beast::zero)
            {
                JLOG (j_.warn())
                    << "deliverNodeForward: NEGATIVE:"
                    << " saTakerPaysNew=" << saTakerPaysNew
                    << " saTakerGetsNew=" << saTakerGetsNew;

                resultCode = telFAILED_PROCESSING;
                break;
            }

            node().sleOffer->setFieldAmount (sfTakerGets, saTakerGetsNew);
            node().sleOffer->setFieldAmount (sfTakerPays, saTakerPaysNew);

            view().update (node().sleOffer);

            if (saOutPassAct == saOutFunded || saTakerGetsNew == beast::zero)
            {

                JLOG (j_.debug())
                    << "deliverNodeForward: unfunded:"
                    << " saOutPassAct=" << saOutPassAct
                    << " saOutFunded=" << saOutFunded;

                pathState_.unfundedOffers().push_back (node().offerIndex_);
                node().bEntryAdvance   = true;
            }
            else
            {
                if (saOutPassAct >= saOutFunded)
                {
                    JLOG (j_.warn())
                        << "deliverNodeForward: TOO MUCH:"
                        << " saOutPassAct=" << saOutPassAct
                        << " saOutFunded=" << saOutFunded;
                }

                assert (saOutPassAct < saOutFunded);
            }

            saInAct += saInPassAct;
            saInFees += saInPassFees;

            node().saFwdDeliver = std::min (node().saRevDeliver,
                                        node().saFwdDeliver + saOutPassAct);
        }
    }

    JLOG (j_.trace())
        << "deliverNodeForward<"
        << " nodeIndex_=" << nodeIndex_
        << " saInAct=" << saInAct
        << " saInFees=" << saInFees;

    return resultCode;
}

} 
} 
























