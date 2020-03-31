

#ifndef RIPPLE_BASICS_RESOLVERASIO_H_INCLUDED
#define RIPPLE_BASICS_RESOLVERASIO_H_INCLUDED

#include <ripple/basics/Resolver.h>
#include <ripple/beast/utility/Journal.h>
#include <boost/asio/io_service.hpp>

namespace ripple {

class ResolverAsio : public Resolver
{
public:
    explicit ResolverAsio() = default;

    static
    std::unique_ptr<ResolverAsio> New (
        boost::asio::io_service&, beast::Journal);
};

}

#endif








