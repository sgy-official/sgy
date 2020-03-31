

#ifndef RIPPLE_BASICS_RANDOM_H_INCLUDED
#define RIPPLE_BASICS_RANDOM_H_INCLUDED

#include <ripple/basics/win32_workaround.h>
#include <ripple/beast/xor_shift_engine.h>
#include <boost/thread/tss.hpp>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <random>
#include <limits>
#include <ripple/beast/cxx17/type_traits.h> 

namespace ripple {

#ifndef __INTELLISENSE__
static_assert (
    std::is_integral <beast::xor_shift_engine::result_type>::value &&
    std::is_unsigned <beast::xor_shift_engine::result_type>::value,
        "The Ripple default PRNG engine must return an unsigned integral type.");

static_assert (
    std::numeric_limits<beast::xor_shift_engine::result_type>::max() >=
    std::numeric_limits<std::uint64_t>::max(),
        "The Ripple default PRNG engine return must be at least 64 bits wide.");
#endif

namespace detail {


template <class Engine, class Result = typename Engine::result_type>
using is_engine =
    std::is_invocable<Engine, Result()>;
}


inline
beast::xor_shift_engine&
default_prng ()
{
    static
    boost::thread_specific_ptr<beast::xor_shift_engine> engine;

    if (!engine.get())
    {
        std::random_device rng;

        std::uint64_t seed = rng();

        for (int i = 0; i < 6; ++i)
        {
            if (seed == 0)
                seed = rng();

            seed ^= (seed << (7 - i)) * rng();
        }

        engine.reset (new beast::xor_shift_engine (seed));
    }

    return *engine;
}



template <class Engine, class Integral>
std::enable_if_t<
    std::is_integral<Integral>::value &&
    detail::is_engine<Engine>::value,
Integral>
rand_int (
    Engine& engine,
    Integral min,
    Integral max)
{
    assert (max > min);

    return std::uniform_int_distribution<Integral>(min, max)(engine);
}

template <class Integral>
std::enable_if_t<std::is_integral<Integral>::value, Integral>
rand_int (
    Integral min,
    Integral max)
{
    return rand_int (default_prng(), min, max);
}

template <class Engine, class Integral>
std::enable_if_t<
    std::is_integral<Integral>::value &&
    detail::is_engine<Engine>::value,
Integral>
rand_int (
    Engine& engine,
    Integral max)
{
    return rand_int (engine, Integral(0), max);
}

template <class Integral>
std::enable_if_t<std::is_integral<Integral>::value, Integral>
rand_int (Integral max)
{
    return rand_int (default_prng(), max);
}

template <class Integral, class Engine>
std::enable_if_t<
    std::is_integral<Integral>::value &&
    detail::is_engine<Engine>::value,
Integral>
rand_int (
    Engine& engine)
{
    return rand_int (
        engine,
        std::numeric_limits<Integral>::max());
}

template <class Integral = int>
std::enable_if_t<std::is_integral<Integral>::value, Integral>
rand_int ()
{
    return rand_int (
        default_prng(),
        std::numeric_limits<Integral>::max());
}




template <class Engine>
inline
bool
rand_bool (Engine& engine)
{
    return rand_int (engine, 1) == 1;
}

inline
bool
rand_bool ()
{
    return rand_bool (default_prng());
}


} 

#endif 








