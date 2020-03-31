

#ifndef RIPPLE_CRYPTO_KEYTYPE_H_INCLUDED
#define RIPPLE_CRYPTO_KEYTYPE_H_INCLUDED

#include <string>
#include <boost/optional.hpp>

namespace ripple {

enum class KeyType
{
    secp256k1 = 0,
    ed25519   = 1,
};

inline
boost::optional<KeyType>
keyTypeFromString (std::string const& s)
{
    if (s == "secp256k1")
        return KeyType::secp256k1;

    if (s == "ed25519")
        return KeyType::ed25519;

    return {};
}

inline
char const*
to_string (KeyType type)
{
    if (type == KeyType::secp256k1)
        return "secp256k1";

    if (type == KeyType::ed25519)
        return "ed25519";

    return "INVALID";
}

template <class Stream>
inline
Stream& operator<<(Stream& s, KeyType type)
{
    return s << to_string(type);
}

}

#endif








