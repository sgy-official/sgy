

#ifndef BEAST_NET_IPADDRESSV4_H_INCLUDED
#define BEAST_NET_IPADDRESSV4_H_INCLUDED

#include <ripple/beast/hash/hash_append.h>
#include <cstdint>
#include <functional>
#include <ios>
#include <string>
#include <utility>
#include <boost/asio/ip/address_v4.hpp>

namespace beast {
namespace IP {

using AddressV4 = boost::asio::ip::address_v4;


bool is_private (AddressV4 const& addr);


bool is_public (AddressV4 const& addr);


char get_class (AddressV4 const& address);

}
}

#endif








