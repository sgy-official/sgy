

#include <ripple/app/misc/LoadFeeTrack.h>
#include <ripple/basics/contract.h>
#include <ripple/basics/Log.h>
#include <ripple/basics/safe_cast.h>
#include <ripple/core/Config.h>
#include <ripple/ledger/ReadView.h>
#include <ripple/protocol/jss.h>
#include <ripple/protocol/STAmount.h>
#include <cstdint>
#include <type_traits>

namespace ripple {

bool
LoadFeeTrack::raiseLocalFee ()
{
    std::lock_guard <std::mutex> sl (lock_);

    if (++raiseCount_ < 2)
        return false;

    std::uint32_t origFee = localTxnLoadFee_;

    if (localTxnLoadFee_ < remoteTxnLoadFee_)
        localTxnLoadFee_ = remoteTxnLoadFee_;

    localTxnLoadFee_ += (localTxnLoadFee_ / lftFeeIncFraction);

    if (localTxnLoadFee_ > lftFeeMax)
        localTxnLoadFee_ = lftFeeMax;

    if (origFee == localTxnLoadFee_)
        return false;

    JLOG(j_.debug()) << "Local load fee raised from " <<
        origFee << " to " << localTxnLoadFee_;
    return true;
}

bool
LoadFeeTrack::lowerLocalFee ()
{
    std::lock_guard <std::mutex> sl (lock_);
    std::uint32_t origFee = localTxnLoadFee_;
    raiseCount_ = 0;

    localTxnLoadFee_ -= (localTxnLoadFee_ / lftFeeDecFraction );

    if (localTxnLoadFee_ < lftNormalFee)
        localTxnLoadFee_ = lftNormalFee;

    if (origFee == localTxnLoadFee_)
        return false;

    JLOG(j_.debug()) << "Local load fee lowered from " <<
        origFee << " to " << localTxnLoadFee_;
    return true;
}



template <class T1, class T2,
    class = std::enable_if_t <
        std::is_integral<T1>::value &&
        std::is_unsigned<T1>::value &&
        sizeof(T1) <= sizeof(std::uint64_t) >,
    class = std::enable_if_t <
        std::is_integral<T2>::value &&
        std::is_unsigned<T2>::value &&
        sizeof(T2) <= sizeof(std::uint64_t) >
>
void lowestTerms(T1& a,  T2& b)
{
    if (a == 0 && b == 0)
        return;

    std::uint64_t x = a, y = b;
    while (y != 0)
    {
        auto t = x % y;
        x = y;
        y = t;
    }
    a /= x;
    b /= x;
}

std::uint64_t
scaleFeeLoad(std::uint64_t fee, LoadFeeTrack const& feeTrack,
    Fees const& fees, bool bUnlimited)
{
    if (fee == 0)
        return fee;
    std::uint32_t feeFactor;
    std::uint32_t uRemFee;
    {
        std::tie(feeFactor, uRemFee) = feeTrack.getScalingFactors();
    }
    if (bUnlimited && (feeFactor > uRemFee) && (feeFactor < (4 * uRemFee)))
        feeFactor = uRemFee;

    auto baseFee = fees.base;

    auto den = safe_cast<std::uint64_t>(fees.units)
        * safe_cast<std::uint64_t>(feeTrack.getLoadBase());
    lowestTerms(fee, den);
    lowestTerms(baseFee, den);
    lowestTerms(feeFactor, den);

    if (fee < baseFee)
        std::swap(fee, baseFee);
    const auto max = std::numeric_limits<std::uint64_t>::max();
    if (baseFee > max / feeFactor)
        Throw<std::overflow_error> ("scaleFeeLoad");
    baseFee *= feeFactor;
    if (fee < baseFee)
        std::swap(fee, baseFee);
    if (fee > max / baseFee)
    {
        fee /= den;
        if (fee > max / baseFee)
            Throw<std::overflow_error> ("scaleFeeLoad");
        fee *= baseFee;
    }
    else
    {
        fee *= baseFee;
        fee /= den;
    }
    return fee;
}

} 
























