

#ifndef RIPPLE_BASICS_CHRONO_H_INCLUDED
#define RIPPLE_BASICS_CHRONO_H_INCLUDED

#include <ripple/beast/clock/abstract_clock.h>
#include <ripple/beast/clock/basic_seconds_clock.h>
#include <ripple/beast/clock/manual_clock.h>
#include <chrono>
#include <cstdint>
#include <string>

namespace ripple {


using days = std::chrono::duration<
    int, std::ratio_multiply<std::chrono::hours::period, std::ratio<24>>>;

using weeks = std::chrono::duration<
    int, std::ratio_multiply<days::period, std::ratio<7>>>;


class NetClock
{
public:
    explicit NetClock() = default;

    using rep        = std::uint32_t;
    using period     = std::ratio<1>;
    using duration   = std::chrono::duration<rep, period>;
    using time_point = std::chrono::time_point<NetClock>;

    static bool const is_steady = false;
};

template <class Duration>
std::string
to_string(date::sys_time<Duration> tp)
{
    return date::format("%Y-%b-%d %T", tp);
}

inline
std::string
to_string(NetClock::time_point tp)
{
    using namespace std::chrono;
    return to_string(
        system_clock::time_point{tp.time_since_epoch() + 946684800s});
}


using Stopwatch = beast::abstract_clock<std::chrono::steady_clock>;


using TestStopwatch = beast::manual_clock<std::chrono::steady_clock>;


inline
Stopwatch&
stopwatch()
{
    return beast::get_abstract_clock<
        std::chrono::steady_clock,
        beast::basic_seconds_clock<std::chrono::steady_clock>>();
}

} 

#endif








