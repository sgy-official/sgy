

#ifndef RIPPLE_TEST_JTX_ACCOUNT_H_INCLUDED
#define RIPPLE_TEST_JTX_ACCOUNT_H_INCLUDED

#include <ripple/protocol/SecretKey.h>
#include <ripple/protocol/UintTypes.h>
#include <ripple/crypto/KeyType.h>
#include <ripple/beast/hash/uhash.h>
#include <unordered_map>
#include <string>

namespace ripple {
namespace test {
namespace jtx {

class IOU;


class Account
{
private:
    struct privateCtorTag{};
public:
    
    static Account const master;

    Account() = default;
    Account (Account&&) = default;
    Account (Account const&) = default;
    Account& operator= (Account const&) = default;
    Account& operator= (Account&&) = default;

    
    
    Account (std::string name,
        KeyType type = KeyType::secp256k1);

    Account (char const* name,
        KeyType type = KeyType::secp256k1)
        : Account(std::string(name), type)
    {
    }

    Account (std::string name,
             std::pair<PublicKey, SecretKey> const& keys, Account::privateCtorTag);

    

    
    std::string const&
    name() const
    {
        return name_;
    }

    
    PublicKey const&
    pk() const
    {
        return pk_;
    }

    
    SecretKey const&
    sk() const
    {
        return sk_;
    }

    
    AccountID
    id() const
    {
        return id_;
    }

    
    std::string const&
    human() const
    {
        return human_;
    }

    
    operator AccountID() const
    {
        return id_;
    }

    
    IOU
    operator[](std::string const& s) const;

private:
    static std::unordered_map<
        std::pair<std::string, KeyType>,
            Account, beast::uhash<>> cache_;


    static Account fromCache(std::string name, KeyType type);

    std::string name_;
    PublicKey pk_;
    SecretKey sk_;
    AccountID id_;
    std::string human_; 
};

inline
bool
operator== (Account const& lhs,
    Account const& rhs) noexcept
{
    return lhs.id() == rhs.id();
}

template <class Hasher>
void
hash_append (Hasher& h,
    Account const& v) noexcept
{
    hash_append(h, v.id());
}

inline
bool
operator< (Account const& lhs,
    Account const& rhs) noexcept
{
    return lhs.id() < rhs.id();
}

} 
} 
} 

#endif








