

#ifndef BEAST_CRYPTO_MAC_FACADE_H_INCLUDED
#define BEAST_CRYPTO_MAC_FACADE_H_INCLUDED

#include <ripple/beast/crypto/secure_erase.h>
#include <ripple/beast/hash/endian.h>
#include <type_traits>
#include <array>

namespace beast {
namespace detail {

template <class Context, bool Secure>
class mac_facade
{
private:
    Context ctx_;

public:
    static beast::endian const endian =
        beast::endian::native;

    static std::size_t const digest_size =
        Context::digest_size;

    using result_type =
        std::array<std::uint8_t, digest_size>;

    mac_facade() noexcept
    {
        init(ctx_);
    }

    ~mac_facade()
    {
        erase(std::integral_constant<
            bool, Secure>{});
    }

    void
    operator()(void const* data,
        std::size_t size) noexcept
    {
        update(ctx_, data, size);
    }

    explicit
    operator result_type() noexcept
    {
        result_type digest;
        finish(ctx_, &digest[0]);
        return digest;
    }

private:
    inline
    void
    erase (std::false_type) noexcept
    {
    }

    inline
    void
    erase (std::true_type) noexcept
    {
        secure_erase(&ctx_, sizeof(ctx_));
    }
};

} 
} 

#endif








