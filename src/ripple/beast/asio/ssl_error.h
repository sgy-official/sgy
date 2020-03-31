

#ifndef BEAST_ASIO_SSL_ERROR_H_INCLUDED
#define BEAST_ASIO_SSL_ERROR_H_INCLUDED

#ifdef _MSC_VER
#  include <winsock2.h>
#endif
#include <boost/asio/ssl/detail/openssl_types.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/lexical_cast.hpp>

namespace beast {


template<class = void>
std::string
error_message_with_ssl(boost::system::error_code const& ec)
{
    std::string error;

    if (ec.category() == boost::asio::error::get_ssl_category())
    {
        error = " ("
            + boost::lexical_cast<std::string>(ERR_GET_LIB (ec.value ()))
            + ","
            + boost::lexical_cast<std::string>(ERR_GET_FUNC (ec.value ()))
            + ","
            + boost::lexical_cast<std::string>(ERR_GET_REASON (ec.value ()))
            + ") ";

        char buf[256];
        ::ERR_error_string_n(ec.value (), buf, sizeof(buf));
        error += buf;
    }
    else
    {
        error = ec.message();
    }

    return error;
}


inline
bool
is_short_read(boost::system::error_code const& ec)
{
#ifdef SSL_R_SHORT_READ
    return (ec.category() == boost::asio::error::get_ssl_category())
        && (ERR_GET_REASON(ec.value()) == SSL_R_SHORT_READ);
#else
    return false;
#endif
}

} 

#endif








