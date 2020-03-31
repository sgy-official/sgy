

#ifndef RIPPLE_SERVER_SERVER_H_INCLUDED
#define RIPPLE_SERVER_SERVER_H_INCLUDED

#include <ripple/server/Port.h>
#include <ripple/server/impl/ServerImpl.h>
#include <ripple/beast/utility/Journal.h>
#include <ripple/beast/utility/PropertyStream.h>
#include <boost/asio/io_service.hpp>

namespace ripple {


template<class Handler>
std::unique_ptr<Server>
make_Server(Handler& handler,
    boost::asio::io_service& io_service, beast::Journal journal)
{
    return std::make_unique<ServerImpl<Handler>>(
        handler, io_service, journal);
}

} 

#endif








