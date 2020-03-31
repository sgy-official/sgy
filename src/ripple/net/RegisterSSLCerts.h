

#ifndef RIPPLE_NET_REGISTER_SSL_CERTS_H_INCLUDED
#define RIPPLE_NET_REGISTER_SSL_CERTS_H_INCLUDED

#include <boost/asio/ssl/context.hpp>
#include <ripple/basics/Log.h>

namespace ripple {

void
registerSSLCerts(
    boost::asio::ssl::context&,
    boost::system::error_code&,
    beast::Journal j);

}  

#endif








