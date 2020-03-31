

#ifndef RIPPLE_PROTOCOL_SECRETKEY_H_INCLUDED
#define RIPPLE_PROTOCOL_SECRETKEY_H_INCLUDED

#include <ripple/basics/Buffer.h>
#include <ripple/basics/Slice.h>
#include <ripple/crypto/KeyType.h> 
#include <ripple/protocol/PublicKey.h>
#include <ripple/protocol/Seed.h>
#include <ripple/protocol/tokens.h>
#include <array>
#include <string>

namespace ripple {


class SecretKey
{
private:
    std::uint8_t buf_[32];

public:
    using const_iterator = std::uint8_t const*;

    SecretKey() = default;
    SecretKey (SecretKey const&) = default;
    SecretKey& operator= (SecretKey const&) = default;

    ~SecretKey();

    SecretKey (std::array<std::uint8_t, 32> const& data);
    SecretKey (Slice const& slice);

    std::uint8_t const*
    data() const
    {
        return buf_;
    }

    std::size_t
    size() const
    {
        return sizeof(buf_);
    }

    
    std::string
    to_string() const;

    const_iterator
    begin() const noexcept
    {
        return buf_;
    }

    const_iterator
    cbegin() const noexcept
    {
        return buf_;
    }

    const_iterator
    end() const noexcept
    {
        return buf_ + sizeof(buf_);
    }

    const_iterator
    cend() const noexcept
    {
        return buf_ + sizeof(buf_);
    }
};

inline
bool
operator== (SecretKey const& lhs,
    SecretKey const& rhs)
{
    return lhs.size() == rhs.size() &&
        std::memcmp(lhs.data(),
            rhs.data(), rhs.size()) == 0;
}

inline
bool
operator!= (SecretKey const& lhs,
    SecretKey const& rhs)
{
    return ! (lhs == rhs);
}



template <>
boost::optional<SecretKey>
parseBase58 (TokenType type, std::string const& s);

inline
std::string
toBase58 (TokenType type, SecretKey const& sk)
{
    return base58EncodeToken(
        type, sk.data(), sk.size());
}


SecretKey
randomSecretKey();


SecretKey
generateSecretKey (KeyType type, Seed const& seed);


PublicKey
derivePublicKey (KeyType type, SecretKey const& sk);


std::pair<PublicKey, SecretKey>
generateKeyPair (KeyType type, Seed const& seed);


std::pair<PublicKey, SecretKey>
randomKeyPair (KeyType type);



Buffer
signDigest (PublicKey const& pk, SecretKey const& sk,
    uint256 const& digest);

inline
Buffer
signDigest (KeyType type, SecretKey const& sk,
    uint256 const& digest)
{
    return signDigest (derivePublicKey(type, sk), sk, digest);
}




Buffer
sign (PublicKey const& pk,
    SecretKey const& sk, Slice const& message);

inline
Buffer
sign (KeyType type, SecretKey const& sk,
    Slice const& message)
{
    return sign (derivePublicKey(type, sk), sk, message);
}


} 

#endif








