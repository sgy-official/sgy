

#include <ripple/app/paths/cursor/EffectiveRate.h>
#include <ripple/app/paths/cursor/RippleLiquidity.h>
#include <ripple/basics/Log.h>

namespace ripple {
namespace path {


TER PathCursor::deliverNodeReverseImpl (
    AccountID const& uOutAccountID, 
    STAmount const& saOutReq,       
    STAmount& saOutAct,             
    bool callerHasLiquidity
                                        ) const
{
    TER resultCode   = tesSUCCESS;

    saOutAct.clear (saOutReq);

    JLOG (j_.trace())
        << "deliverNodeReverse>"
        << " saOutAct=" << saOutAct
        << " saOutReq=" << saOutReq
        << " saPrvDlvReq=" << previousNode().saRevDeliver;

    assert (saOutReq != beast::zero);

    int loopCount = 0;
    auto viewJ = rippleCalc_.logs_.journal ("View");

    while (saOutAct < saOutReq)
    {
        if (++loopCount >
            (multiQuality_ ?
                CALC_NODE_DELIVER_MAX_LOOPS_MQ :
                CALC_NODE_DELIVER_MAX_LOOPS))
        {
            JLOG (j_.warn()) << "loop count exceeded";
            return telFAILED_PROCESSING;
        }

        resultCode = advanceNode (saOutAct, true, callerHasLiquidity);

        if (resultCode != tesSUCCESS || !node().offerIndex_)
            break;

        auto const xferRate = effectiveRate (
            node().issue_,
            uOutAccountID,
            node().offerOwnerAccount_,
            node().transferRate_);

        JLOG (j_.trace())
            << "deliverNodeReverse:"
            << " offerOwnerAccount_=" << node().offerOwnerAccount_
            << " uOutAccountID=" << uOutAccountID
            << " node().issue_.account=" << node().issue_.account
            << " xferRate=" << xferRate;

        if (!multiQuality_)
        {
            if (!node().rateMax)
            {
                JLOG (j_.trace())
                    << "Set initial rate";

                node().rateMax = xferRate;
            }
            else if (xferRate > node().rateMax)
            {
                JLOG (j_.trace())
                    << "Offer exceeds initial rate: " << *node().rateMax;

                break;  
            }
            else if (xferRate < node().rateMax)
            {

                JLOG (j_.trace())
                    << "Reducing rate: " << *node().rateMax;

                node().rateMax = xferRate;
            }
        }

        STAmount saOutPassReq = std::min (
            std::min (node().saOfferFunds, node().saTakerGets),
            saOutReq - saOutAct);

        STAmount saOutPassAct = saOutPassReq;

        STAmount saOutPlusFees   = multiplyRound (
            saOutPassAct, xferRate, false);



        JLOG (j_.trace())
            << "deliverNodeReverse:"
            << " saOutReq=" << saOutReq
            << " saOutAct=" << saOutAct
            << " node().saTakerGets=" << node().saTakerGets
            << " saOutPassAct=" << saOutPassAct
            << " saOutPlusFees=" << saOutPlusFees
            << " node().saOfferFunds=" << node().saOfferFunds;

        if (saOutPlusFees > node().saOfferFunds)
        {
            saOutPlusFees   = node().saOfferFunds;

            auto fee = divideRound (saOutPlusFees, xferRate, true);
            saOutPassAct = std::min (saOutPassReq, fee);

            JLOG (j_.trace())
                << "deliverNodeReverse: Total exceeds fees:"
                << " saOutPassAct=" << saOutPassAct
                << " saOutPlusFees=" << saOutPlusFees
                << " node().saOfferFunds=" << node().saOfferFunds;
        }

        auto outputFee = mulRound (
            saOutPassAct, node().saOfrRate, node().saTakerPays.issue (), true);
        if (*stAmountCalcSwitchover == false && ! outputFee)
        {
            JLOG (j_.fatal())
                << "underflow computing outputFee "
                << "saOutPassAct: " << saOutPassAct
                << " saOfrRate: " << node ().saOfrRate;
            return telFAILED_PROCESSING;
        }
        STAmount saInPassReq = std::min (node().saTakerPays, outputFee);
        STAmount saInPassAct;

        JLOG (j_.trace())
            << "deliverNodeReverse:"
            << " outputFee=" << outputFee
            << " saInPassReq=" << saInPassReq
            << " node().saOfrRate=" << node().saOfrRate
            << " saOutPassAct=" << saOutPassAct
            << " saOutPlusFees=" << saOutPlusFees;

        if (!saInPassReq) 
        {
            JLOG (j_.debug())
                << "deliverNodeReverse: micro offer is unfunded.";

            node().bEntryAdvance   = true;
            continue;
        }
        else if (!isXRP(previousNode().account_))
        {

            saInPassAct = saInPassReq;

            JLOG (j_.trace())
                << "deliverNodeReverse: account --> OFFER --> ? :"
                << " saInPassAct=" << saInPassAct;
        }
        else
        {

            resultCode = increment(-1).deliverNodeReverseImpl(
                node().offerOwnerAccount_,
                saInPassReq,
                saInPassAct,
                saOutAct > beast::zero);

            if (fix1141(view().info().parentCloseTime))
            {
                if (resultCode == tecPATH_DRY && saOutAct > beast::zero)
                {
                    resultCode = tesSUCCESS;
                    break;
                }
            }

            JLOG (j_.trace())
                << "deliverNodeReverse: offer --> OFFER --> ? :"
                << " saInPassAct=" << saInPassAct;
        }

        if (resultCode != tesSUCCESS)
            break;

        if (saInPassAct < saInPassReq)
        {
            auto outputRequirements = divRound (saInPassAct, node ().saOfrRate,
                node ().saTakerGets.issue (), true);
            saOutPassAct = std::min (saOutPassReq, outputRequirements);
            auto outputFees = multiplyRound (saOutPassAct, xferRate, true);
            saOutPlusFees   = std::min (node().saOfferFunds, outputFees);

            JLOG (j_.trace())
                << "deliverNodeReverse: adjusted:"
                << " saOutPassAct=" << saOutPassAct
                << " saOutPlusFees=" << saOutPlusFees;
        }
        else
        {
            assert (saInPassAct == saInPassReq);
        }

        node().bFundsDirty = true;

        resultCode   = accountSend(view(),
            node().offerOwnerAccount_, node().issue_.account, saOutPassAct, viewJ);

        if (resultCode != tesSUCCESS)
            break;

        STAmount saTakerGetsNew  = node().saTakerGets - saOutPassAct;
        STAmount saTakerPaysNew  = node().saTakerPays - saInPassAct;

        if (saTakerPaysNew < beast::zero || saTakerGetsNew < beast::zero)
        {
            JLOG (j_.warn())
                << "deliverNodeReverse: NEGATIVE:"
                << " node().saTakerPaysNew=" << saTakerPaysNew
                << " node().saTakerGetsNew=" << saTakerGetsNew;

            resultCode = telFAILED_PROCESSING;
            break;
        }

        node().sleOffer->setFieldAmount (sfTakerGets, saTakerGetsNew);
        node().sleOffer->setFieldAmount (sfTakerPays, saTakerPaysNew);

        view().update (node().sleOffer);

        if (saOutPassAct == node().saTakerGets)
        {
            JLOG (j_.debug())
                << "deliverNodeReverse: offer became unfunded.";

            node().bEntryAdvance   = true;
        }
        else
        {
            assert (saOutPassAct < node().saTakerGets);
        }

        saOutAct += saOutPassAct;
        previousNode().saRevDeliver += saInPassAct;
    }

    if (saOutAct > saOutReq)
    {
        JLOG (j_.warn())
            << "deliverNodeReverse: TOO MUCH:"
            << " saOutAct=" << saOutAct
            << " saOutReq=" << saOutReq;
    }

    assert(saOutAct <= saOutReq);

    if (resultCode == tesSUCCESS && !saOutAct)
        resultCode = tecPATH_DRY;

    JLOG (j_.trace())
        << "deliverNodeReverse<"
        << " saOutAct=" << saOutAct
        << " saOutReq=" << saOutReq
        << " saPrvDlvReq=" << previousNode().saRevDeliver;

    return resultCode;
}

TER PathCursor::deliverNodeReverse (
    AccountID const& uOutAccountID, 
    STAmount const& saOutReq,       
    STAmount& saOutAct              
                                    ) const
{
    for (int i = nodeIndex_; i >= 0 && !node (i).isAccount(); --i)
        node (i).directory.restart (multiQuality_);

    return deliverNodeReverseImpl(uOutAccountID, saOutReq, saOutAct, false);
}

}  
}  
























