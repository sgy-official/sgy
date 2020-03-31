

#ifndef RIPPLE_APP_LEDGER_LEDGERTIMING_H_INCLUDED
#define RIPPLE_APP_LEDGER_LEDGERTIMING_H_INCLUDED

#include <ripple/basics/chrono.h>
#include <ripple/beast/utility/Journal.h>
#include <chrono>
#include <cstdint>

namespace ripple {


std::chrono::seconds constexpr ledgerPossibleTimeResolutions[] =
    {
        std::chrono::seconds { 10},
        std::chrono::seconds { 20},
        std::chrono::seconds { 30},
        std::chrono::seconds { 60},
        std::chrono::seconds { 90},
        std::chrono::seconds {120}
    };

auto constexpr ledgerDefaultTimeResolution = ledgerPossibleTimeResolutions[2];

auto constexpr increaseLedgerTimeResolutionEvery = 8;

auto constexpr decreaseLedgerTimeResolutionEvery = 1;


template <class Rep, class Period, class Seq>
std::chrono::duration<Rep, Period>
getNextLedgerTimeResolution(
    std::chrono::duration<Rep, Period> previousResolution,
    bool previousAgree,
    Seq ledgerSeq)
{
    assert(ledgerSeq != Seq{0});

    using namespace std::chrono;
    auto iter = std::find(
        std::begin(ledgerPossibleTimeResolutions),
        std::end(ledgerPossibleTimeResolutions),
        previousResolution);
    assert(iter != std::end(ledgerPossibleTimeResolutions));

    if (iter == std::end(ledgerPossibleTimeResolutions))
        return previousResolution;

    if (!previousAgree &&
        (ledgerSeq % Seq{decreaseLedgerTimeResolutionEvery} == Seq{0}))
    {
        if (++iter != std::end(ledgerPossibleTimeResolutions))
            return *iter;
    }

    if (previousAgree &&
        (ledgerSeq % Seq{increaseLedgerTimeResolutionEvery} == Seq{0}))
    {
        if (iter-- != std::begin(ledgerPossibleTimeResolutions))
            return *iter;
    }

    return previousResolution;
}


template <class Clock, class Duration, class Rep, class Period>
std::chrono::time_point<Clock, Duration>
roundCloseTime(
    std::chrono::time_point<Clock, Duration> closeTime,
    std::chrono::duration<Rep, Period> closeResolution)
{
    using time_point = decltype(closeTime);
    if (closeTime == time_point{})
        return closeTime;

    closeTime += (closeResolution / 2);
    return closeTime - (closeTime.time_since_epoch() % closeResolution);
}


template <class Clock, class Duration, class Rep, class Period>
std::chrono::time_point<Clock, Duration>
effCloseTime(
    std::chrono::time_point<Clock, Duration> closeTime,
    std::chrono::duration<Rep, Period> resolution,
    std::chrono::time_point<Clock, Duration> priorCloseTime)
{
    using namespace std::chrono_literals;
    using time_point = decltype(closeTime);

    if (closeTime == time_point{})
        return closeTime;

    return std::max<time_point>(
        roundCloseTime(closeTime, resolution), (priorCloseTime + 1s));
}

}
#endif








