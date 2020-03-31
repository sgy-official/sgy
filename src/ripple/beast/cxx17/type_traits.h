

#ifndef BEAST_CXX17_TYPE_TRAITS_H_INCLUDED
#define BEAST_CXX17_TYPE_TRAITS_H_INCLUDED

#include <tuple>
#include <type_traits>
#include <utility>

namespace std {

#ifndef _MSC_VER

    #if ! __cpp_lib_void_t

        template<class...>
        using void_t = void;

    #endif 

    #if ! __cpp_lib_bool_constant

        template<bool B>
        using bool_constant = std::integral_constant<bool, B>;

    #endif 

#endif

template <class T, class U>
struct is_constructible <pair <T, U>>
    : integral_constant <bool,
        is_default_constructible <T>::value &&
        is_default_constructible <U>::value>
{
    explicit is_constructible() = default;
};


namespace detail {
template<class R, class C, class ...A>
auto
is_invocable_test(C&& c, int, A&& ...a)
        -> decltype(std::is_convertible<
                    decltype(c(std::forward<A>(a)...)), R>::value ||
                    std::is_same<R, void>::value,
                    std::true_type());

template<class R, class C, class ...A>
std::false_type
is_invocable_test(C&& c, long, A&& ...a);
} 



template<class C, class F>
struct is_invocable : std::false_type
{
};

template<class C, class R, class ...A>
struct is_invocable<C, R(A...)>
        : decltype(std::detail::is_invocable_test<R>(
            std::declval<C>(), 1, std::declval<A>()...))
{
};



} 

#endif








