

#ifndef RIPPLE_PROTOCOL_UINTTYPES_H_INCLUDED
#define RIPPLE_PROTOCOL_UINTTYPES_H_INCLUDED

#include <ripple/basics/UnorderedContainers.h>
#include <ripple/basics/base_uint.h>
#include <ripple/protocol/AccountID.h>
#include <ripple/beast/utility/Zero.h>

namespace ripple {
namespace detail {

class CurrencyTag
{
public:
    explicit CurrencyTag() = default;
};

class DirectoryTag
{
public:
    explicit DirectoryTag() = default;
};

class NodeIDTag
{
public:
    explicit NodeIDTag() = default;
};

} 


using Directory = base_uint<256, detail::DirectoryTag>;


using Currency = base_uint<160, detail::CurrencyTag>;


using NodeID = base_uint<160, detail::NodeIDTag>;


Currency const& xrpCurrency();


Currency const& noCurrency();


Currency const& badCurrency();

inline bool isXRP(Currency const& c)
{
    return c == beast::zero;
}


std::string to_string(Currency const& c);


bool to_currency(Currency&, std::string const&);


Currency to_currency(std::string const&);

inline std::ostream& operator<< (std::ostream& os, Currency const& x)
{
    os << to_string (x);
    return os;
}

} 

namespace std {

template <>
struct hash <ripple::Currency> : ripple::Currency::hasher
{
    explicit hash() = default;
};

template <>
struct hash <ripple::NodeID> : ripple::NodeID::hasher
{
    explicit hash() = default;
};

template <>
struct hash <ripple::Directory> : ripple::Directory::hasher
{
    explicit hash() = default;
};

} 

#endif








