

#ifndef RIPPLE_PROTOCOL_ACCOUNTID_H_INCLUDED
#define RIPPLE_PROTOCOL_ACCOUNTID_H_INCLUDED

#include <ripple/protocol/tokens.h>
#include <ripple/basics/base_uint.h>
#include <ripple/basics/UnorderedContainers.h>
#include <ripple/json/json_value.h>
#include <boost/optional.hpp>
#include <cstddef>
#include <mutex>
#include <string>

namespace ripple {

namespace detail {

class AccountIDTag
{
public:
    explicit AccountIDTag() = default;
};

} 


using AccountID = base_uint<
    160, detail::AccountIDTag>;


std::string
toBase58 (AccountID const& v);


template<>
boost::optional<AccountID>
parseBase58 (std::string const& s);

boost::optional<AccountID>
deprecatedParseBitcoinAccountID (std::string const& s);

bool
deprecatedParseBase58 (AccountID& account,
    Json::Value const& jv);


template<>
boost::optional<AccountID>
parseHex (std::string const& s);


template<>
boost::optional<AccountID>
parseHexOrBase58 (std::string const& s);




AccountID const&
xrpAccount();


AccountID const&
noAccount();


bool
to_issuer (AccountID&, std::string const&);

inline
bool
isXRP(AccountID const& c)
{
    return c == beast::zero;
}

inline
std::string
to_string (AccountID const& account)
{
    return toBase58(account);
}

inline std::ostream& operator<< (std::ostream& os, AccountID const& x)
{
    os << to_string (x);
    return os;
}



class AccountIDCache
{
private:
    std::mutex mutable mutex_;
    std::size_t capacity_;
    hash_map<AccountID,
        std::string> mutable m0_;
    hash_map<AccountID,
        std::string> mutable m1_;

public:
    AccountIDCache(AccountIDCache const&) = delete;
    AccountIDCache& operator= (AccountIDCache const&) = delete;

    explicit
    AccountIDCache (std::size_t capacity);

    
    std::string
    toBase58 (AccountID const&) const;
};

} 


namespace std {

template <>
struct hash <ripple::AccountID> : ripple::AccountID::hasher
{
    explicit hash() = default;
};

} 

#endif








