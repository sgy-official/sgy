

#include <ripple/app/paths/Flow.h>
#include <ripple/app/paths/RippleCalc.h>
#include <ripple/app/paths/Tuning.h>
#include <ripple/app/paths/cursor/PathCursor.h>
#include <ripple/app/paths/impl/FlowDebugInfo.h>
#include <ripple/basics/Log.h>
#include <ripple/ledger/View.h>
#include <ripple/protocol/Feature.h>

namespace ripple {
namespace path {

static
TER
deleteOffers (ApplyView& view,
    boost::container::flat_set<uint256> const& offers, beast::Journal j)
{
    for (auto& e: offers)
        if (TER r = offerDelete (view,
                view.peek(keylet::offer(e)), j))
            return r;
    return tesSUCCESS;
}

RippleCalc::Output RippleCalc::rippleCalculate (
    PaymentSandbox& view,


    STAmount const& saMaxAmountReq,             

    STAmount const& saDstAmountReq,

    AccountID const& uDstAccountID,
    AccountID const& uSrcAccountID,

    STPathSet const& spsPaths,
    Logs& l,
    Input const* const pInputs)
{
    bool const compareFlowV1V2 =
        view.rules ().enabled (featureCompareFlowV1V2);

    bool const useFlowV1Output =
        !view.rules().enabled(featureFlow);
    bool const callFlowV1 = useFlowV1Output || compareFlowV1V2;
    bool const callFlowV2 = !useFlowV1Output || compareFlowV1V2;

    Output flowV1Out;
    PaymentSandbox flowV1SB (&view);

    auto const inNative = saMaxAmountReq.native();
    auto const outNative = saDstAmountReq.native();
    detail::FlowDebugInfo flowV1FlowDebugInfo (inNative, outNative);
    if (callFlowV1)
    {
        auto const timeIt = flowV1FlowDebugInfo.timeBlock ("main");
        RippleCalc rc (
            flowV1SB,
            saMaxAmountReq,
            saDstAmountReq,
            uDstAccountID,
            uSrcAccountID,
            spsPaths,
            l);
        if (pInputs != nullptr)
        {
            rc.inputFlags = *pInputs;
        }

        auto result = rc.rippleCalculate (compareFlowV1V2 ? &flowV1FlowDebugInfo : nullptr);
        flowV1Out.setResult (result);
        flowV1Out.actualAmountIn = rc.actualAmountIn_;
        flowV1Out.actualAmountOut = rc.actualAmountOut_;
        if (result != tesSUCCESS && !rc.permanentlyUnfundedOffers_.empty ())
            flowV1Out.removableOffers = std::move (rc.permanentlyUnfundedOffers_);
    }

    Output flowV2Out;
    PaymentSandbox flowV2SB (&view);
    detail::FlowDebugInfo flowV2FlowDebugInfo (inNative, outNative);
    auto j = l.journal ("Flow");
    if (callFlowV2)
    {
        bool defaultPaths = true;
        bool partialPayment = false;
        boost::optional<Quality> limitQuality;
        boost::optional<STAmount> sendMax;

        if (pInputs)
        {
            defaultPaths = pInputs->defaultPathsAllowed;
            partialPayment = pInputs->partialPaymentAllowed;
            if (pInputs->limitQuality && saMaxAmountReq > beast::zero)
                limitQuality.emplace (
                    Amounts (saMaxAmountReq, saDstAmountReq));
        }

        if (saMaxAmountReq >= beast::zero ||
            saMaxAmountReq.getCurrency () != saDstAmountReq.getCurrency () ||
            saMaxAmountReq.getIssuer () != uSrcAccountID)
        {
            sendMax.emplace (saMaxAmountReq);
        }

        try
        {
            bool const ownerPaysTransferFee =
                    view.rules ().enabled (featureOwnerPaysFee);
            auto const timeIt = flowV2FlowDebugInfo.timeBlock ("main");
            flowV2Out = flow (flowV2SB, saDstAmountReq, uSrcAccountID,
                uDstAccountID, spsPaths, defaultPaths, partialPayment,
                ownerPaysTransferFee,  false, limitQuality, sendMax, j,
                compareFlowV1V2 ? &flowV2FlowDebugInfo : nullptr);
        }
        catch (std::exception& e)
        {
            JLOG (j.error()) << "Exception from flow: " << e.what ();
            if (!useFlowV1Output)
            {
                path::RippleCalc::Output exceptResult;
                exceptResult.setResult(tecINTERNAL);
                return exceptResult;
            }
        }
    }

    if (j.debug())
    {
        using BalanceDiffs = detail::BalanceDiffs;
        auto logResult = [&](std::string const& algoName,
            Output const& result,
            detail::FlowDebugInfo const& flowDebugInfo,
            boost::optional<BalanceDiffs> const& balanceDiffs,
            bool outputPassInfo,
            bool outputBalanceDiffs) {
                j.debug () << "RippleCalc Result> " <<
                " actualIn: " << result.actualAmountIn <<
                ", actualOut: " << result.actualAmountOut <<
                ", result: " << result.result () <<
                ", dstAmtReq: " << saDstAmountReq <<
                ", sendMax: " << saMaxAmountReq <<
                (compareFlowV1V2 ? ", " + flowDebugInfo.to_string (outputPassInfo): "") <<
                (outputBalanceDiffs && balanceDiffs
                 ? ", " + detail::balanceDiffsToString(balanceDiffs)  : "") <<
                ", algo: " << algoName;
        };
        bool outputPassInfo = false;
        bool outputBalanceDiffs = false;
        boost::optional<BalanceDiffs> bdV1, bdV2;
        if (compareFlowV1V2)
        {
            auto const v1r = flowV1Out.result ();
            auto const v2r = flowV2Out.result ();
            if (v1r != v2r ||
                (((v1r == tesSUCCESS) || (v1r == tecPATH_PARTIAL)) &&
                    ((flowV1Out.actualAmountIn !=
                         flowV2Out.actualAmountIn) ||
                        (flowV1Out.actualAmountOut !=
                            flowV2Out.actualAmountOut))))
            {
                outputPassInfo = true;
            }
            bdV1 = detail::balanceDiffs (flowV1SB, view);
            bdV2 = detail::balanceDiffs (flowV2SB, view);
            outputBalanceDiffs = bdV1 != bdV2;
        }

        if (callFlowV1)
        {
            logResult ("V1", flowV1Out, flowV1FlowDebugInfo, bdV1,
                outputPassInfo, outputBalanceDiffs);
        }
        if (callFlowV2)
        {
            logResult ("V2", flowV2Out, flowV2FlowDebugInfo, bdV2,
                outputPassInfo, outputBalanceDiffs);
        }
    }

    JLOG (j.trace()) << "Using old flow: " << useFlowV1Output;

    if (!useFlowV1Output)
    {
        flowV2SB.apply (view);
        return flowV2Out;
    }
    flowV1SB.apply (view);
    return flowV1Out;
}

bool RippleCalc::addPathState(STPath const& path, TER& resultCode)
{
    auto pathState = std::make_shared<PathState> (
        view, saDstAmountReq_, saMaxAmountReq_, j_);

    if (!pathState)
    {
        resultCode = temUNKNOWN;
        return false;
    }

    pathState->expandPath (
        path,
        uDstAccountID_,
        uSrcAccountID_);

    if (pathState->status() == tesSUCCESS)
        pathState->checkNoRipple (uDstAccountID_, uSrcAccountID_);

    if (pathState->status() == tesSUCCESS)
        pathState->checkFreeze ();

    pathState->setIndex (pathStateList_.size ());

    JLOG (j_.debug())
        << "rippleCalc: Build direct:"
        << " status: " << transToken (pathState->status());

    if (isTemMalformed (pathState->status()))
    {
        resultCode = pathState->status();
        return false;
    }

    if (pathState->status () == tesSUCCESS)
    {
        resultCode = pathState->status();
        pathStateList_.push_back (pathState);
    }
    else if (pathState->status () != terNO_LINE)
    {
        resultCode = pathState->status();
    }

    return true;
}


TER RippleCalc::rippleCalculate (detail::FlowDebugInfo* flowDebugInfo)
{
    JLOG (j_.trace())
        << "rippleCalc>"
        << " saMaxAmountReq_:" << saMaxAmountReq_
        << " saDstAmountReq_:" << saDstAmountReq_;

    TER resultCode = temUNCERTAIN;
    permanentlyUnfundedOffers_.clear ();
    mumSource_.clear ();


    if (inputFlags.defaultPathsAllowed)
    {
        if (!addPathState (STPath(), resultCode))
            return resultCode;
    }
    else if (spsPaths_.empty ())
    {
        JLOG (j_.debug())
            << "rippleCalc: Invalid transaction:"
            << "No paths and direct ripple not allowed.";

        return temRIPPLE_EMPTY;
    }


    JLOG (j_.trace())
        << "rippleCalc: Paths in set: " << spsPaths_.size ();

    for (auto const& spPath: spsPaths_)
    {
        if (!addPathState (spPath, resultCode))
            return resultCode;
    }

    if (resultCode != tesSUCCESS)
        return (resultCode == temUNCERTAIN) ? terNO_LINE : resultCode;

    resultCode = temUNCERTAIN;

    actualAmountIn_ = saMaxAmountReq_.zeroed();
    actualAmountOut_ = saDstAmountReq_.zeroed();

    const std::uint64_t uQualityLimit = inputFlags.limitQuality ?
            getRate (saDstAmountReq_, saMaxAmountReq_) : 0;

    boost::container::flat_set<uint256> unfundedOffersFromBestPaths;

    int iPass = 0;
    auto const dcSwitch = fix1141(view.info().parentCloseTime);

    while (resultCode == temUNCERTAIN)
    {
        int iBest = -1;
        int iDry = 0;

        bool multiQuality = false;

        if (flowDebugInfo) flowDebugInfo->newLiquidityPass();
        for (auto pathState : pathStateList_)
        {
            if (pathState->quality())
            {
                multiQuality = dcSwitch
                    ? !inputFlags.limitQuality &&
                        ((pathStateList_.size () - iDry) == 1)
                    : ((pathStateList_.size () - iDry) == 1);

                pathState->reset (actualAmountIn_, actualAmountOut_);

                PathCursor pc(*this, *pathState, multiQuality, j_);
                pc.nextIncrement ();

                JLOG (j_.debug())
                    << "rippleCalc: AFTER:"
                    << " mIndex=" << pathState->index()
                    << " uQuality=" << pathState->quality()
                    << " rate=" << amountFromQuality (pathState->quality());

                if (flowDebugInfo)
                    flowDebugInfo->pushLiquiditySrc (
                        toEitherAmount (pathState->inPass ()),
                        toEitherAmount (pathState->outPass ()));

                if (!pathState->quality())
                {

                    ++iDry;
                }
                else if (pathState->outPass() == beast::zero)
                {

                    JLOG (j_.warn())
                        << "rippleCalc: Non-dry path moves no funds";

                    assert (false);

                    pathState->setQuality (0);
                    ++iDry;
                }
                else
                {
                    if (!pathState->inPass() || !pathState->outPass())
                    {
                        JLOG (j_.debug())
                            << "rippleCalc: better:"
                            << " uQuality="
                            << amountFromQuality (pathState->quality())
                            << " inPass()=" << pathState->inPass()
                            << " saOutPass=" << pathState->outPass();
                    }

                    assert (pathState->inPass() && pathState->outPass());

                    JLOG (j_.debug())
                        << "Old flow iter (iter, in, out): "
                        << iPass << " "
                        << pathState->inPass() << " "
                        << pathState->outPass();

                    if ((!inputFlags.limitQuality ||
                         pathState->quality() <= uQualityLimit)
                        && (iBest < 0
                            || PathState::lessPriority (
                                *pathStateList_[iBest], *pathState)))
                    {
                        JLOG (j_.debug())
                            << "rippleCalc: better:"
                            << " mIndex=" << pathState->index()
                            << " uQuality=" << pathState->quality()
                            << " rate="
                            << amountFromQuality (pathState->quality())
                            << " inPass()=" << pathState->inPass()
                            << " saOutPass=" << pathState->outPass();

                        iBest   = pathState->index ();
                    }
                }
            }
        }

        ++iPass;

        if (auto stream = j_.debug())
        {
            stream
                << "rippleCalc: Summary:"
                << " Pass: " << iPass
                << " Dry: " << iDry
                << " Paths: " << pathStateList_.size ();
            for (auto pathState: pathStateList_)
            {
                stream
                    << "rippleCalc: "
                    << "Summary: " << pathState->index()
                    << " rate: "
                    << amountFromQuality (pathState->quality())
                    << " quality:" << pathState->quality()
                    << " best: " << (iBest == pathState->index ());
            }
        }

        if (iBest >= 0)
        {
            auto pathState = pathStateList_[iBest];

            if (flowDebugInfo)
                flowDebugInfo->pushPass (toEitherAmount (pathState->inPass ()),
                    toEitherAmount (pathState->outPass ()),
                    pathStateList_.size () - iDry);

            JLOG (j_.debug ())
                << "rippleCalc: best:"
                << " uQuality=" << amountFromQuality (pathState->quality ())
                << " inPass()=" << pathState->inPass ()
                << " saOutPass=" << pathState->outPass () << " iBest=" << iBest;


            unfundedOffersFromBestPaths.insert (
                pathState->unfundedOffers().begin (),
                pathState->unfundedOffers().end ());

            pathState->view().apply(view);

            actualAmountIn_ += pathState->inPass();
            actualAmountOut_ += pathState->outPass();

            JLOG (j_.trace())
                    << "rippleCalc: best:"
                    << " uQuality="
                    << amountFromQuality (pathState->quality())
                    << " inPass()=" << pathState->inPass()
                    << " saOutPass=" << pathState->outPass()
                    << " actualIn=" << actualAmountIn_
                    << " actualOut=" << actualAmountOut_
                    << " iBest=" << iBest;

            if (multiQuality)
            {
                ++iDry;
                pathState->setQuality(0);
            }

            if (actualAmountOut_ == saDstAmountReq_)
            {

                resultCode   = tesSUCCESS;
            }
            else if (actualAmountOut_ > saDstAmountReq_)
            {
                JLOG (j_.fatal())
                    << "rippleCalc: TOO MUCH:"
                    << " actualAmountOut_:" << actualAmountOut_
                    << " saDstAmountReq_:" << saDstAmountReq_;

                return tefEXCEPTION;  
                assert (false);
            }
            else if (actualAmountIn_ != saMaxAmountReq_ &&
                     iDry != pathStateList_.size ())
            {
                mumSource_.insert (
                    pathState->reverse().begin (), pathState->reverse().end ());

                if (iPass >= PAYMENT_MAX_LOOPS)
                {

                    JLOG (j_.error())
                       << "rippleCalc: pass limit";

                    resultCode = telFAILED_PROCESSING;
                }

            }
            else if (!inputFlags.partialPaymentAllowed)
            {

                resultCode   = tecPATH_PARTIAL;
            }
            else
            {

                resultCode   = tesSUCCESS;
            }
        }
        else if (!inputFlags.partialPaymentAllowed)
        {
            resultCode = tecPATH_PARTIAL;
        }
        else if (!actualAmountOut_)
        {
            resultCode = tecPATH_DRY;
        }
        else
        {
            resultCode   = tesSUCCESS;
        }
    }

    if (resultCode == tesSUCCESS)
    {
        auto viewJ = logs_.journal ("View");
        resultCode = deleteOffers(view, unfundedOffersFromBestPaths, viewJ);
        if (resultCode == tesSUCCESS)
            resultCode = deleteOffers(view, permanentlyUnfundedOffers_, viewJ);
    }

    if (resultCode == telFAILED_PROCESSING && !inputFlags.isLedgerOpen)
        return tecFAILED_PROCESSING;
    return resultCode;
}

} 
} 
























