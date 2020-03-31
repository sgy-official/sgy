

#ifndef RIPPLE_PROTOCOL_RATE_H_INCLUDED
#define RIPPLE_PROTOCOL_RATE_H_INCLUDED

#include <ripple/protocol/STAmount.h>
#include <boost/operators.hpp>
#include <cassert>
#include <cstdint>
#include <ostream>

namespace ripple {


struct Rate
    : private boost::totally_ordered <Rate>
{
    std::uint32_t value;

    Rate () = delete;

    explicit
    Rate (std::uint32_t rate)
        : value (rate)
    {
    }
};

inline
bool
operator== (Rate const& lhs, Rate const& rhs) noexcept
{
    return lhs.value == rhs.value;
}

inline
bool
operator< (Rate const& lhs, Rate const& rhs) noexcept
{
    return lhs.value < rhs.value;
}

inline
std::ostream&
operator<< (std::ostream& os, Rate const& rate)
{
    os << rate.value;
    return os;
}

STAmount
multiply (
    STAmount const& amount,
    Rate const& rate);

STAmount
multiplyRound (
    STAmount const& amount,
    Rate const& rate,
    bool roundUp);

STAmount
multiplyRound (
    STAmount const& amount,
    Rate const& rate,
    Issue const& issue,
    bool roundUp);

STAmount
divide (
    STAmount const& amount,
    Rate const& rate);

STAmount
divideRound (
    STAmount const& amount,
    Rate const& rate,
    bool roundUp);

STAmount
divideRound (
    STAmount const& amount,
    Rate const& rate,
    Issue const& issue,
    bool roundUp);


extern Rate const parityRate;

}

#endif








