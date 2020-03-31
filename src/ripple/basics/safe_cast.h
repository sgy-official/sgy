

#ifndef RIPPLE_BASICS_SAFE_CAST_H_INCLUDED
#define RIPPLE_BASICS_SAFE_CAST_H_INCLUDED

#include <type_traits>

namespace ripple {


template <class Dest, class Src>
inline
constexpr
std::enable_if_t
<
    std::is_integral<Dest>::value && std::is_integral<Src>::value,
    Dest
>
safe_cast(Src s) noexcept
{
    static_assert(std::is_signed<Dest>::value || std::is_unsigned<Src>::value,
        "Cannot cast signed to unsigned");
    constexpr unsigned not_same = std::is_signed<Dest>::value !=
                                  std::is_signed<Src>::value;
    static_assert(sizeof(Dest) >= sizeof(Src) + not_same,
        "Destination is too small to hold all values of source");
    return static_cast<Dest>(s);
}

template <class Dest, class Src>
inline
constexpr
std::enable_if_t
<
    std::is_enum<Dest>::value && std::is_integral<Src>::value,
    Dest
>
safe_cast(Src s) noexcept
{
    return static_cast<Dest>(safe_cast<std::underlying_type_t<Dest>>(s));
}

template <class Dest, class Src>
inline
constexpr
std::enable_if_t
<
    std::is_integral<Dest>::value && std::is_enum<Src>::value,
    Dest
>
safe_cast(Src s) noexcept
{
    return safe_cast<Dest>(static_cast<std::underlying_type_t<Src>>(s));
}

} 

#endif








