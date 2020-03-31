

#include <ripple/protocol/Quality.h>
#include <ripple/app/paths/cursor/RippleLiquidity.h>
#include <ripple/basics/Log.h>

namespace ripple {
namespace path {


void rippleLiquidity (
    RippleCalc& rippleCalc,
    Rate const& qualityIn,
    Rate const& qualityOut,
    STAmount const& saPrvReq,   
    STAmount const& saCurReq,   
    STAmount& saPrvAct,  
    STAmount& saCurAct,  
    std::uint64_t& uRateMax)
{
    JLOG (rippleCalc.j_.trace())
        << "rippleLiquidity>"
        << " qualityIn=" << qualityIn
        << " qualityOut=" << qualityOut
        << " saPrvReq=" << saPrvReq
        << " saCurReq=" << saCurReq
        << " saPrvAct=" << saPrvAct
        << " saCurAct=" << saCurAct;

    assert (saCurReq != beast::zero);
    assert (saCurReq > beast::zero);

    assert (saPrvReq.getCurrency () == saCurReq.getCurrency ());
    assert (saPrvReq.getCurrency () == saPrvAct.getCurrency ());
    assert (saPrvReq.getIssuer () == saPrvAct.getIssuer ());

    const bool bPrvUnlimited = (saPrvReq < beast::zero);  


    const STAmount saPrv = bPrvUnlimited ? saPrvReq : saPrvReq - saPrvAct;

    const STAmount  saCur = saCurReq - saCurAct;

    JLOG (rippleCalc.j_.trace())
        << "rippleLiquidity: "
        << " bPrvUnlimited=" << bPrvUnlimited
        << " saPrv=" << saPrv
        << " saCur=" << saCur;

    if (saPrv == beast::zero || saCur == beast::zero)
        return;

    if (qualityIn >= qualityOut)
    {
        JLOG (rippleCalc.j_.trace()) << "rippleLiquidity: No fees";

        if (!uRateMax || STAmount::uRateOne <= uRateMax)
        {
            STAmount saTransfer = bPrvUnlimited ? saCur
                : std::min (saPrv, saCur);

            saPrvAct += saTransfer;
            saCurAct += saTransfer;

            if (uRateMax == 0)
                uRateMax = STAmount::uRateOne;
        }
    }
    else
    {
        JLOG (rippleCalc.j_.trace()) << "rippleLiquidity: Fee";

        std::uint64_t const uRate = getRate (
            STAmount (qualityOut.value),
            STAmount (qualityIn.value));

        if (!uRateMax || uRate <= uRateMax)
        {
            auto numerator = multiplyRound (saCur, qualityOut, true);

            STAmount saCurIn = divideRound (numerator, qualityIn, true);

            JLOG (rippleCalc.j_.trace())
                << "rippleLiquidity:"
                << " bPrvUnlimited=" << bPrvUnlimited
                << " saPrv=" << saPrv
                << " saCurIn=" << saCurIn;

            if (bPrvUnlimited || saCurIn <= saPrv)
            {
                saCurAct += saCur;
                saPrvAct += saCurIn;
                JLOG (rippleCalc.j_.trace())
                    << "rippleLiquidity:3c:"
                    << " saCurReq=" << saCurReq
                    << " saPrvAct=" << saPrvAct;
            }
            else
            {
                auto numerator = multiplyRound (saPrv,
                    qualityIn, saCur.issue(), true);
                STAmount saCurOut = divideRound (numerator,
                    qualityOut, saCur.issue(), true);

                JLOG (rippleCalc.j_.trace())
                    << "rippleLiquidity:4: saCurReq=" << saCurReq;

                saCurAct += saCurOut;
                saPrvAct = saPrvReq;
            }
            if (!uRateMax)
                uRateMax = uRate;
        }
    }

    JLOG (rippleCalc.j_.trace())
        << "rippleLiquidity<"
        << " qualityIn=" << qualityIn
        << " qualityOut=" << qualityOut
        << " saPrvReq=" << saPrvReq
        << " saCurReq=" << saCurReq
        << " saPrvAct=" << saPrvAct
        << " saCurAct=" << saCurAct;
}

static
Rate
rippleQuality (
    ReadView const& view,
    AccountID const& destination,
    AccountID const& source,
    Currency const& currency,
    SField const& sfLow,
    SField const& sfHigh)
{
    if (destination == source)
        return parityRate;

    auto const& sfField = destination < source ? sfLow : sfHigh;

    auto const sle = view.read(
        keylet::line(destination, source, currency));

    if (!sle || !sle->isFieldPresent (sfField))
        return parityRate;

    auto quality = sle->getFieldU32 (sfField);

    if (quality == 0)
        quality = 1;

    return Rate{ quality };
}

Rate
quality_in (
    ReadView const& view,
    AccountID const& uToAccountID,
    AccountID const& uFromAccountID,
    Currency const& currency)
{
    return rippleQuality (view, uToAccountID, uFromAccountID, currency,
        sfLowQualityIn, sfHighQualityIn);
}

Rate
quality_out (
    ReadView const& view,
    AccountID const& uToAccountID,
    AccountID const& uFromAccountID,
    Currency const& currency)
{
    return rippleQuality (view, uToAccountID, uFromAccountID, currency,
        sfLowQualityOut, sfHighQualityOut);
}

} 
} 
























