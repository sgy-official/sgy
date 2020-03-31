

#ifndef BEAST_NET_IPADDRESSV6_H_INCLUDED
#define BEAST_NET_IPADDRESSV6_H_INCLUDED

#include <cassert>
#include <cstdint>
#include <functional>
#include <ios>
#include <string>
#include <utility>
#include <boost/asio/ip/address_v6.hpp>

namespace beast {
namespace IP {

using AddressV6 = boost::asio::ip::address_v6;


bool is_private (AddressV6 const& addr);


bool is_public (AddressV6 const& addr);

}
}

#endif








