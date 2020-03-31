

#ifndef BEAST_CRYPTO_RIPEMD_H_INCLUDED
#define BEAST_CRYPTO_RIPEMD_H_INCLUDED

#include <ripple/beast/crypto/detail/mac_facade.h>
#include <ripple/beast/crypto/detail/ripemd_context.h>

namespace beast {

using ripemd160_hasher = detail::mac_facade<
    detail::ripemd160_context, false>;

using ripemd160_hasher_s = detail::mac_facade<
    detail::ripemd160_context, true>;

}

#endif








