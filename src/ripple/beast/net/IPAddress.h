

#ifndef BEAST_NET_IPADDRESS_H_INCLUDED
#define BEAST_NET_IPADDRESS_H_INCLUDED

#include <ripple/beast/net/IPAddressV4.h>
#include <ripple/beast/net/IPAddressV6.h>
#include <ripple/beast/hash/hash_append.h>
#include <ripple/beast/hash/uhash.h>
#include <boost/functional/hash.hpp>
#include <boost/asio/ip/address.hpp>
#include <cassert>
#include <cstdint>
#include <ios>
#include <string>
#include <sstream>
#include <typeinfo>


namespace beast {
namespace IP {

using Address = boost::asio::ip::address;


inline
std::string
to_string (Address const& addr)
{
    return addr.to_string ();
}


inline
bool
is_loopback (Address const& addr)
{
    return addr.is_loopback();
}


inline
bool
is_unspecified (Address const& addr)
{
    return addr.is_unspecified();
}


inline
bool
is_multicast (Address const& addr)
{
    return addr.is_multicast();
}


inline
bool
is_private (Address const& addr)
{
    return (addr.is_v4 ())
        ? is_private (addr.to_v4 ())
        : is_private (addr.to_v6 ());
}


inline
bool
is_public (Address const& addr)
{
    return (addr.is_v4 ())
        ? is_public (addr.to_v4 ())
        : is_public (addr.to_v6 ());
}

}


template <class Hasher>
void
hash_append(Hasher& h, beast::IP::Address const& addr) noexcept
{
    using beast::hash_append;
    if (addr.is_v4 ())
        hash_append(h, addr.to_v4().to_bytes());
    else if (addr.is_v6 ())
        hash_append(h, addr.to_v6().to_bytes());
    else
        assert (false);
}
}

namespace std {
template <>
struct hash <beast::IP::Address>
{
    explicit hash() = default;

    std::size_t
    operator() (beast::IP::Address const& addr) const
    {
        return beast::uhash<>{} (addr);
    }
};
}

namespace boost {
template <>
struct hash <::beast::IP::Address>
{
    explicit hash() = default;

    std::size_t
    operator() (::beast::IP::Address const& addr) const
    {
        return ::beast::uhash<>{} (addr);
    }
};
}

#endif








