

#ifndef RIPPLE_BASICS_HARDENED_HASH_H_INCLUDED
#define RIPPLE_BASICS_HARDENED_HASH_H_INCLUDED

#include <ripple/beast/hash/hash_append.h>
#include <ripple/beast/hash/xxhasher.h>

#include <cstdint>
#include <functional>
#include <mutex>
#include <utility>
#include <random>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>

#ifndef RIPPLE_NO_HARDENED_HASH_INSTANCE_SEED
# ifdef __GLIBCXX__
#  define RIPPLE_NO_HARDENED_HASH_INSTANCE_SEED 1
# else
#  define RIPPLE_NO_HARDENED_HASH_INSTANCE_SEED 0
# endif
#endif

namespace ripple {

namespace detail {

using seed_pair = std::pair<std::uint64_t, std::uint64_t>;

template <bool = true>
seed_pair
make_seed_pair() noexcept
{
    struct state_t
    {
        std::mutex mutex;
        std::random_device rng;
        std::mt19937_64 gen;
        std::uniform_int_distribution <std::uint64_t> dist;

        state_t() : gen(rng()) {}
    };
    static state_t state;
    std::lock_guard <std::mutex> lock (state.mutex);
    return {state.dist(state.gen), state.dist(state.gen)};
}

}

template <class HashAlgorithm, bool ProcessSeeded>
class basic_hardened_hash;


template <class HashAlgorithm>
class basic_hardened_hash <HashAlgorithm, true>
{
private:
    static
    detail::seed_pair const&
    init_seed_pair()
    {
        static detail::seed_pair const p = detail::make_seed_pair<>();
        return p;
    }

public:
    explicit basic_hardened_hash() = default;

    using result_type = typename HashAlgorithm::result_type;

    template <class T>
    result_type
    operator()(T const& t) const noexcept
    {
        std::uint64_t seed0;
        std::uint64_t seed1;
        std::tie(seed0, seed1) = init_seed_pair();
        HashAlgorithm h(seed0, seed1);
        hash_append(h, t);
        return static_cast<result_type>(h);
    }
};


template <class HashAlgorithm>
class basic_hardened_hash<HashAlgorithm, false>
{
private:
    detail::seed_pair m_seeds;

public:
    using result_type = typename HashAlgorithm::result_type;

    basic_hardened_hash()
        : m_seeds (detail::make_seed_pair<>())
    {}

    template <class T>
    result_type
    operator()(T const& t) const noexcept
    {
        HashAlgorithm h(m_seeds.first, m_seeds.second);
        hash_append(h, t);
        return static_cast<result_type>(h);
    }
};



#if RIPPLE_NO_HARDENED_HASH_INSTANCE_SEED
template <class HashAlgorithm = beast::xxhasher>
    using hardened_hash = basic_hardened_hash<HashAlgorithm, true>;
#else
template <class HashAlgorithm = beast::xxhasher>
    using hardened_hash = basic_hardened_hash<HashAlgorithm, false>;
#endif

} 

#endif








