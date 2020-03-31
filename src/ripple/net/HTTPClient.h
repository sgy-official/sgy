

#ifndef RIPPLE_NET_HTTPCLIENT_H_INCLUDED
#define RIPPLE_NET_HTTPCLIENT_H_INCLUDED

#include <ripple/basics/ByteUtilities.h>
#include <ripple/core/Config.h>
#include <boost/asio/io_service.hpp>
#include <boost/asio/streambuf.hpp>
#include <chrono>

namespace ripple {


class HTTPClient
{
public:
    explicit HTTPClient() = default;

    static constexpr auto maxClientHeaderBytes = kilobytes(32);

    static void initializeSSLContext (Config const& config, beast::Journal j);

    static void get (
        bool bSSL,
        boost::asio::io_service& io_service,
        std::deque <std::string> deqSites,
        const unsigned short port,
        std::string const& strPath,
        std::size_t responseMax,    
        std::chrono::seconds timeout,
        std::function <bool (const boost::system::error_code& ecResult, int iStatus, std::string const& strData)> complete,
        beast::Journal& j);

    static void get (
        bool bSSL,
        boost::asio::io_service& io_service,
        std::string strSite,
        const unsigned short port,
        std::string const& strPath,
        std::size_t responseMax,    
        std::chrono::seconds timeout,
        std::function <bool (const boost::system::error_code& ecResult, int iStatus, std::string const& strData)> complete,
        beast::Journal& j);

    static void request (
        bool bSSL,
        boost::asio::io_service& io_service,
        std::string strSite,
        const unsigned short port,
        std::function <void (boost::asio::streambuf& sb, std::string const& strHost)> build,
        std::size_t responseMax,    
        std::chrono::seconds timeout,
        std::function <bool (const boost::system::error_code& ecResult, int iStatus, std::string const& strData)> complete,
        beast::Journal& j);
};

} 

#endif








