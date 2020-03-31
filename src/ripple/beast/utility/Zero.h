

#ifndef BEAST_UTILITY_ZERO_H_INCLUDED
#define BEAST_UTILITY_ZERO_H_INCLUDED

namespace beast {



struct Zero
{
    explicit Zero() = default;
};

namespace {
static constexpr Zero zero{};
}


template <typename T>
auto signum(T const& t)
{
    return t.signum();
}

namespace detail {
namespace zero_helper {

template <class T>
auto call_signum(T const& t)
{
    return signum(t);
}

} 
} 


template <typename T>
bool operator==(T const& t, Zero)
{
    return detail::zero_helper::call_signum(t) == 0;
}

template <typename T>
bool operator!=(T const& t, Zero)
{
    return detail::zero_helper::call_signum(t) != 0;
}

template <typename T>
bool operator<(T const& t, Zero)
{
    return detail::zero_helper::call_signum(t) < 0;
}

template <typename T>
bool operator>(T const& t, Zero)
{
    return detail::zero_helper::call_signum(t) > 0;
}

template <typename T>
bool operator>=(T const& t, Zero)
{
    return detail::zero_helper::call_signum(t) >= 0;
}

template <typename T>
bool operator<=(T const& t, Zero)
{
    return detail::zero_helper::call_signum(t) <= 0;
}


template <typename T>
bool operator==(Zero, T const& t)
{
    return t == zero;
}

template <typename T>
bool operator!=(Zero, T const& t)
{
    return t != zero;
}

template <typename T>
bool operator<(Zero, T const& t)
{
    return t > zero;
}

template <typename T>
bool operator>(Zero, T const& t)
{
    return t < zero;
}

template <typename T>
bool operator>=(Zero, T const& t)
{
    return t <= zero;
}

template <typename T>
bool operator<=(Zero, T const& t)
{
    return t >= zero;
}

} 

#endif








