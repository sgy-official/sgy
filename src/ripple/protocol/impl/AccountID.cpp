

#include <ripple/protocol/AccountID.h>
#include <ripple/protocol/PublicKey.h>
#include <ripple/protocol/digest.h>
#include <ripple/protocol/tokens.h>
#include <cstring>

namespace ripple {

std::string
toBase58 (AccountID const& v)
{
    return base58EncodeToken(TokenType::AccountID, v.data(), v.size());
}

template<>
boost::optional<AccountID>
parseBase58 (std::string const& s)
{
    auto const result = decodeBase58Token(s, TokenType::AccountID);
    if (result.empty())
        return boost::none;
    AccountID id;
    if (result.size() != id.size())
        return boost::none;
    std::memcpy(id.data(),
        result.data(), result.size());
    return id;
}

boost::optional<AccountID>
deprecatedParseBitcoinAccountID (std::string const& s)
{
    auto const result = decodeBase58TokenBitcoin(s, TokenType::AccountID);
    if (result.empty())
        return boost::none;
    AccountID id;
    if (result.size() != id.size())
        return boost::none;
    std::memcpy(id.data(),
        result.data(), result.size());
    return id;
}

bool
deprecatedParseBase58 (AccountID& account,
    Json::Value const& jv)
{
    if (! jv.isString())
        return false;
    auto const result =
        parseBase58<AccountID>(jv.asString());
    if (! result)
        return false;
    account = *result;
    return true;
}

template<>
boost::optional<AccountID>
parseHex (std::string const& s)
{
    if (s.size() != 40)
        return boost::none;
    AccountID id;
    if (! id.SetHex(s, true))
        return boost::none;
    return id;
}

template<>
boost::optional<AccountID>
parseHexOrBase58 (std::string const& s)
{
    auto result =
        parseHex<AccountID>(s);
    if (! result)
        result = parseBase58<AccountID>(s);
    return result;
}


AccountID
calcAccountID (PublicKey const& pk)
{
    ripesha_hasher rsh;
    rsh(pk.data(), pk.size());
    auto const d = static_cast<
        ripesha_hasher::result_type>(rsh);
    AccountID id;
    static_assert(sizeof(d) == id.size(), "");
    std::memcpy(id.data(), d.data(), d.size());
    return id;
}

AccountID const&
xrpAccount()
{
    static AccountID const account(beast::zero);
    return account;
}

AccountID const&
noAccount()
{
    static AccountID const account(1);
    return account;
}

bool
to_issuer (AccountID& issuer, std::string const& s)
{
    if (s.size () == (160 / 4))
    {
        issuer.SetHex (s);
        return true;
    }
    auto const account =
        parseBase58<AccountID>(s);
    if (! account)
        return false;
    issuer = *account;
    return true;
}




AccountIDCache::AccountIDCache(
        std::size_t capacity)
    : capacity_(capacity)
{
    m1_.reserve(capacity_);
}

std::string
AccountIDCache::toBase58(
    AccountID const& id) const
{
    std::lock_guard<
        std::mutex> lock(mutex_);
    auto iter = m1_.find(id);
    if (iter != m1_.end())
        return iter->second;
    iter = m0_.find(id);
    std::string result;
    if (iter != m0_.end())
    {
        result = iter->second;
        m0_.erase(iter);
    }
    else
    {
        result =
            ripple::toBase58(id);
    }
    if (m1_.size() >= capacity_)
    {
        m0_ = std::move(m1_);
        m1_.clear();
        m1_.reserve(capacity_);
    }
    m1_.emplace(id, result);
    return result;
}

} 
























