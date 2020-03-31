

#ifndef RIPPLE_PROTOCOL_PUBLICKEY_H_INCLUDED
#define RIPPLE_PROTOCOL_PUBLICKEY_H_INCLUDED

#include <ripple/basics/Slice.h>
#include <ripple/crypto/KeyType.h> 
#include <ripple/protocol/STExchange.h>
#include <ripple/protocol/tokens.h>
#include <ripple/protocol/UintTypes.h>
#include <boost/optional.hpp>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <ostream>
#include <utility>

namespace ripple {


class PublicKey
{
protected:
    std::size_t size_ = 0;
    std::uint8_t buf_[33]; 

public:
    using const_iterator = std::uint8_t const*;

    PublicKey() = default;
    PublicKey (PublicKey const& other);
    PublicKey& operator= (PublicKey const& other);

    
    explicit
    PublicKey (Slice const& slice);

    std::uint8_t const*
    data() const noexcept
    {
        return buf_;
    }

    std::size_t
    size() const noexcept
    {
        return size_;
    }

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
        return buf_ + size_;
    }

    const_iterator
    cend() const noexcept
    {
        return buf_ + size_;
    }

    bool
    empty() const noexcept
    {
        return size_ == 0;
    }

    Slice
    slice() const noexcept
    {
        return { buf_, size_ };
    }

    operator Slice() const noexcept
    {
        return slice();
    }
};


std::ostream&
operator<<(std::ostream& os, PublicKey const& pk);

inline
bool
operator== (PublicKey const& lhs,
    PublicKey const& rhs)
{
    return lhs.size() == rhs.size() &&
        std::memcmp(lhs.data(), rhs.data(), rhs.size()) == 0;
}

inline
bool
operator< (PublicKey const& lhs,
    PublicKey const& rhs)
{
    return std::lexicographical_compare(
        lhs.data(), lhs.data() + lhs.size(),
        rhs.data(), rhs.data() + rhs.size());
}

template <class Hasher>
void
hash_append (Hasher& h,
    PublicKey const& pk)
{
    h(pk.data(), pk.size());
}

template<>
struct STExchange<STBlob, PublicKey>
{
    explicit STExchange() = default;

    using value_type = PublicKey;

    static
    void
    get (boost::optional<value_type>& t,
        STBlob const& u)
    {
        t.emplace (Slice(u.data(), u.size()));
    }

    static
    std::unique_ptr<STBlob>
    set (SField const& f, PublicKey const& t)
    {
        return std::make_unique<STBlob>(
            f, t.data(), t.size());
    }
};


inline
std::string
toBase58 (TokenType type, PublicKey const& pk)
{
    return base58EncodeToken(
        type, pk.data(), pk.size());
}

template<>
boost::optional<PublicKey>
parseBase58 (TokenType type, std::string const& s);

enum class ECDSACanonicality
{
    canonical,
    fullyCanonical
};


boost::optional<ECDSACanonicality>
ecdsaCanonicality (Slice const& sig);



boost::optional<KeyType>
publicKeyType (Slice const& slice);

inline
boost::optional<KeyType>
publicKeyType (PublicKey const& publicKey)
{
    return publicKeyType (publicKey.slice());
}



bool
verifyDigest (PublicKey const& publicKey,
    uint256 const& digest,
    Slice const& sig,
    bool mustBeFullyCanonical = true);


bool
verify (PublicKey const& publicKey,
    Slice const& m,
    Slice const& sig,
    bool mustBeFullyCanonical = true);


NodeID
calcNodeID (PublicKey const&);

AccountID
calcAccountID (PublicKey const& pk);

} 

#endif








