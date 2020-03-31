

#ifndef RIPPLE_BASICS_RESOLVER_H_INCLUDED
#define RIPPLE_BASICS_RESOLVER_H_INCLUDED

#include <vector>
#include <functional>

#include <ripple/beast/net/IPEndpoint.h>

namespace ripple {

class Resolver
{
public:
    using HandlerType = std::function <
        void (std::string, std::vector <beast::IP::Endpoint>) >;

    virtual ~Resolver () = 0;

    
    virtual void stop_async () = 0;

    
    virtual void stop () = 0;

    
    virtual void start () = 0;

    
    
    template <class Handler>
    void resolve (std::vector <std::string> const& names, Handler handler)
    {
        resolve (names, HandlerType (handler));
    }

    virtual void resolve (
        std::vector <std::string> const& names,
        HandlerType const& handler) = 0;
    
};

}

#endif








