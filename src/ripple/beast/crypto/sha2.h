

#ifndef BEAST_CRYPTO_SHA2_H_INCLUDED
#define BEAST_CRYPTO_SHA2_H_INCLUDED

#include <ripple/beast/crypto/detail/mac_facade.h>
#include <ripple/beast/crypto/detail/sha2_context.h>

namespace beast {

using sha256_hasher = detail::mac_facade<
    detail::sha256_context, false>;

using sha256_hasher_s = detail::mac_facade<
    detail::sha256_context, true>;

using sha512_hasher = detail::mac_facade<
    detail::sha512_context, false>;

using sha512_hasher_s = detail::mac_facade<
    detail::sha512_context, true>;

}

#endif








