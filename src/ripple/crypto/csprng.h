

#ifndef RIPPLE_CRYPTO_RANDOM_H_INCLUDED
#define RIPPLE_CRYPTO_RANDOM_H_INCLUDED

#include <mutex>
#include <string>
#include <type_traits>

namespace ripple {


class csprng_engine
{
private:
    std::mutex mutex_;

    void
    mix (
        void* buffer,
        std::size_t count,
        double bitsPerByte);

public:
    using result_type = std::uint64_t;

    csprng_engine(csprng_engine const&) =  delete;
    csprng_engine& operator=(csprng_engine const&) = delete;

    csprng_engine(csprng_engine&&) = delete;
    csprng_engine& operator=(csprng_engine&&) = delete;

    csprng_engine ();
    ~csprng_engine ();

    
    void
    mix_entropy (void* buffer = nullptr, std::size_t count = 0);

    
    result_type
    operator()();

    
    void
    operator()(void *ptr, std::size_t count);

    
    static constexpr
    result_type
    min()
    {
        return std::numeric_limits<result_type>::min();
    }

    
    static constexpr
    result_type
    max()
    {
        return std::numeric_limits<result_type>::max();
    }
};


csprng_engine& crypto_prng();

}

#endif








