

#ifndef BEAST_HASH_META_H_INCLUDED
#define BEAST_HASH_META_H_INCLUDED

#include <type_traits>

namespace beast {

template <bool ...> struct static_and;

template <bool b0, bool ... bN>
struct static_and <b0, bN...>
    : public std::integral_constant <
        bool, b0 && static_and<bN...>::value>
{
    explicit static_and() = default;
};

template <>
struct static_and<>
    : public std::true_type
{
    explicit static_and() = default;
};

#ifndef __INTELLISENSE__
static_assert( static_and<true, true, true>::value, "");
static_assert(!static_and<true, false, true>::value, "");
#endif

template <std::size_t ...>
struct static_sum;

template <std::size_t s0, std::size_t ...sN>
struct static_sum <s0, sN...>
    : public std::integral_constant <
        std::size_t, s0 + static_sum<sN...>::value>
{
    explicit static_sum() = default;
};

template <>
struct static_sum<>
    : public std::integral_constant<std::size_t, 0>
{
    explicit static_sum() = default;
};

#ifndef __INTELLISENSE__
static_assert(static_sum<5, 2, 17, 0>::value == 24, "");
#endif

template <class T, class U>
struct enable_if_lvalue
    : public std::enable_if
    <
    std::is_same<std::decay_t<T>, U>::value &&
    std::is_lvalue_reference<T>::value
    >
{
    explicit enable_if_lvalue() = default;
};


template <class T, class U>
using enable_if_lvalue_t = typename enable_if_lvalue<T, U>::type;

} 

#endif 








