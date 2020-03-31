

#include <ripple/basics/contract.h>
#include <ripple/basics/ByteUtilities.h>
#include <ripple/crypto/csprng.h>
#include <openssl/rand.h>
#include <array>
#include <cassert>
#include <random>
#include <stdexcept>

namespace ripple {

void
csprng_engine::mix (
    void* data, std::size_t size, double bitsPerByte)
{
    assert (data != nullptr);
    assert (size != 0);
    assert (bitsPerByte != 0);

    std::lock_guard<std::mutex> lock (mutex_);
    RAND_add (data, size, (size * bitsPerByte) / 8.0);
}

csprng_engine::csprng_engine ()
{
    mix_entropy ();
}

csprng_engine::~csprng_engine ()
{
    RAND_cleanup ();
}

void
csprng_engine::mix_entropy (void* buffer, std::size_t count)
{
    std::array<std::random_device::result_type, 128> entropy;

    {
        std::random_device rd;

        for (auto& e : entropy)
            e = rd();
    }

    mix (
        entropy.data(),
        entropy.size() * sizeof(std::random_device::result_type),
        2.0);

    if (buffer != nullptr && count != 0)
        mix (buffer, count, 0.5);
}

csprng_engine::result_type
csprng_engine::operator()()
{
    result_type ret;

    std::lock_guard<std::mutex> lock (mutex_);

    auto const result = RAND_bytes (
        reinterpret_cast<unsigned char*>(&ret),
        sizeof(ret));

    if (result == 0)
        Throw<std::runtime_error> ("Insufficient entropy");

    return ret;
}

void
csprng_engine::operator()(void *ptr, std::size_t count)
{
    std::lock_guard<std::mutex> lock (mutex_);

    auto const result = RAND_bytes (
        reinterpret_cast<unsigned char*>(ptr),
        count);

    if (result != 1)
        Throw<std::runtime_error> ("Insufficient entropy");
}

csprng_engine& crypto_prng()
{
    static csprng_engine engine;
    return engine;
}

}
























