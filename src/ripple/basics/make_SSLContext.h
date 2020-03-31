

#ifndef RIPPLE_BASICS_MAKE_SSLCONTEXT_H_INCLUDED
#define RIPPLE_BASICS_MAKE_SSLCONTEXT_H_INCLUDED

#include <boost/asio/ssl/context.hpp>
#include <string>

namespace ripple {


std::shared_ptr<boost::asio::ssl::context>
make_SSLContext(
    std::string const& cipherList);


std::shared_ptr<boost::asio::ssl::context>
make_SSLContextAuthed (
    std::string const& keyFile,
    std::string const& certFile,
    std::string const& chainFile,
    std::string const& cipherList);


}

#endif








