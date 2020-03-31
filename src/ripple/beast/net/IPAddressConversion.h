

#ifndef BEAST_NET_IPADDRESSCONVERSION_H_INCLUDED
#define BEAST_NET_IPADDRESSCONVERSION_H_INCLUDED

#include <ripple/beast/net/IPEndpoint.h>

#include <sstream>

#include <boost/asio.hpp>

namespace beast {
namespace IP {


Endpoint from_asio (boost::asio::ip::address const& address);


Endpoint from_asio (boost::asio::ip::tcp::endpoint const& endpoint);


boost::asio::ip::address to_asio_address (Endpoint const& endpoint);


boost::asio::ip::tcp::endpoint to_asio_endpoint (Endpoint const& endpoint);

}
}

namespace beast {

struct IPAddressConversion
{
    explicit IPAddressConversion() = default;

    static IP::Endpoint from_asio (boost::asio::ip::address const& address)
        { return IP::from_asio (address); }
    static IP::Endpoint from_asio (boost::asio::ip::tcp::endpoint const& endpoint)
        { return IP::from_asio (endpoint); }
    static boost::asio::ip::address to_asio_address (IP::Endpoint const& address)
        { return IP::to_asio_address (address); }
    static boost::asio::ip::tcp::endpoint to_asio_endpoint (IP::Endpoint const& address)
        { return IP::to_asio_endpoint (address); }
};

}

#endif








