

#ifndef RIPPLE_SERVER_PORT_H_INCLUDED
#define RIPPLE_SERVER_PORT_H_INCLUDED

#include <ripple/basics/BasicConfig.h>
#include <ripple/beast/net/IPEndpoint.h>
#include <boost/beast/core/string.hpp>
#include <boost/beast/websocket/option.hpp>
#include <boost/asio/ip/address.hpp>
#include <cstdint>
#include <memory>
#include <set>
#include <string>

namespace boost { namespace asio { namespace ssl { class context; } } }

namespace ripple {


struct Port
{
    explicit Port() = default;

    std::string name;
    boost::asio::ip::address ip;
    std::uint16_t port = 0;
    std::set<std::string, boost::beast::iless> protocol;
    std::vector<beast::IP::Address> admin_ip;
    std::vector<beast::IP::Address> secure_gateway_ip;
    std::string user;
    std::string password;
    std::string admin_user;
    std::string admin_password;
    std::string ssl_key;
    std::string ssl_cert;
    std::string ssl_chain;
    std::string ssl_ciphers;
    boost::beast::websocket::permessage_deflate pmd_options;
    std::shared_ptr<boost::asio::ssl::context> context;

    int limit = 0;

    std::uint16_t ws_queue_limit;

    bool websockets() const;

    bool secure() const;

    std::string protocols() const;
};

std::ostream&
operator<< (std::ostream& os, Port const& p);


struct ParsedPort
{
    explicit ParsedPort() = default;

    std::string name;
    std::set<std::string, boost::beast::iless> protocol;
    std::string user;
    std::string password;
    std::string admin_user;
    std::string admin_password;
    std::string ssl_key;
    std::string ssl_cert;
    std::string ssl_chain;
    std::string ssl_ciphers;
    boost::beast::websocket::permessage_deflate pmd_options;
    int limit = 0;
    std::uint16_t ws_queue_limit;

    boost::optional<boost::asio::ip::address> ip;
    boost::optional<std::uint16_t> port;
    boost::optional<std::vector<beast::IP::Address>> admin_ip;
    boost::optional<std::vector<beast::IP::Address>> secure_gateway_ip;
};

void
parse_Port (ParsedPort& port, Section const& section, std::ostream& log);

} 

#endif








