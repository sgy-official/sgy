

#ifndef RIPPLE_PROTOCOL_DIGEST_H_INCLUDED
#define RIPPLE_PROTOCOL_DIGEST_H_INCLUDED

#include <ripple/basics/base_uint.h>
#include <ripple/beast/crypto/ripemd.h>
#include <ripple/beast/crypto/sha2.h>
#include <ripple/beast/hash/endian.h>
#include <algorithm>
#include <array>

namespace ripple {





struct openssl_ripemd160_hasher
{
public:
    static beast::endian const endian =
        beast::endian::native;

    using result_type =
        std::array<std::uint8_t, 20>;

    openssl_ripemd160_hasher();

    void
    operator()(void const* data,
        std::size_t size) noexcept;

    explicit
    operator result_type() noexcept;

private:
    char ctx_[96];
};


struct openssl_sha512_hasher
{
public:
    static beast::endian const endian =
        beast::endian::native;

    using result_type =
        std::array<std::uint8_t, 64>;

    openssl_sha512_hasher();

    void
    operator()(void const* data,
        std::size_t size) noexcept;

    explicit
    operator result_type() noexcept;

private:
    char ctx_[216];
};


struct openssl_sha256_hasher
{
public:
    static beast::endian const endian =
        beast::endian::native;

    using result_type =
        std::array<std::uint8_t, 32>;

    openssl_sha256_hasher();

    void
    operator()(void const* data,
        std::size_t size) noexcept;

    explicit
    operator result_type() noexcept;

private:
    char ctx_[112];
};



#if USE_BEAST_HASHER
using ripemd160_hasher = beast::ripemd160_hasher;
using sha256_hasher = beast::sha256_hasher;
using sha512_hasher = beast::sha512_hasher;
#else
using ripemd160_hasher = openssl_ripemd160_hasher;
using sha256_hasher = openssl_sha256_hasher;
using sha512_hasher = openssl_sha512_hasher;
#endif



struct ripesha_hasher
{
private:
    sha256_hasher h_;

public:
    static beast::endian const endian =
        beast::endian::native;

    using result_type =
        std::array<std::uint8_t, 20>;

    void
    operator()(void const* data,
        std::size_t size) noexcept
    {
        h_(data, size);
    }

    explicit
    operator result_type() noexcept
    {
        auto const d0 =
            sha256_hasher::result_type(h_);
        ripemd160_hasher rh;
        rh(d0.data(), d0.size());
        return ripemd160_hasher::result_type(rh);
    }
};


namespace detail {


template <bool Secure>
struct basic_sha512_half_hasher
{
private:
    sha512_hasher h_;

public:
    static beast::endian const endian =
        beast::endian::big;

    using result_type = uint256;

    ~basic_sha512_half_hasher()
    {
        erase(std::integral_constant<
            bool, Secure>{});
    }

    void
    operator()(void const* data,
        std::size_t size) noexcept
    {
        h_(data, size);
    }

    explicit
    operator result_type() noexcept
    {
        auto const digest =
            sha512_hasher::result_type(h_);
        result_type result;
        std::copy(digest.begin(),
            digest.begin() + 32, result.begin());
        return result;
    }

private:
    inline
    void
    erase (std::false_type)
    {
    }

    inline
    void
    erase (std::true_type)
    {
        beast::secure_erase(&h_, sizeof(h_));
    }
};

} 

using sha512_half_hasher =
    detail::basic_sha512_half_hasher<false>;

using sha512_half_hasher_s =
    detail::basic_sha512_half_hasher<true>;


#ifdef _MSC_VER
inline
void
sha512_deprecatedMSVCWorkaround()
{
    beast::sha512_hasher h;
    auto const digest = static_cast<
        beast::sha512_hasher::result_type>(h);
}
#endif


template <class... Args>
sha512_half_hasher::result_type
sha512Half (Args const&... args)
{
    sha512_half_hasher h;
    using beast::hash_append;
    hash_append(h, args...);
    return static_cast<typename
        sha512_half_hasher::result_type>(h);
}


template <class... Args>
sha512_half_hasher_s::result_type
sha512Half_s (Args const&... args)
{
    sha512_half_hasher_s h;
    using beast::hash_append;
    hash_append(h, args...);
    return static_cast<typename
        sha512_half_hasher_s::result_type>(h);
}

} 

#endif








