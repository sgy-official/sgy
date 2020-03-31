

#ifndef RIPPLE_RESOURCE_MANAGER_H_INCLUDED
#define RIPPLE_RESOURCE_MANAGER_H_INCLUDED

#include <ripple/json/json_value.h>
#include <ripple/resource/Consumer.h>
#include <ripple/resource/Gossip.h>
#include <ripple/beast/insight/Collector.h>
#include <ripple/beast/net/IPEndpoint.h>
#include <ripple/beast/utility/Journal.h>
#include <ripple/beast/utility/PropertyStream.h>
#include <boost/utility/string_view.hpp>

namespace ripple {
namespace Resource {


class Manager : public beast::PropertyStream::Source
{
protected:
    Manager ();

public:
    virtual ~Manager() = 0;

    
    virtual Consumer newInboundEndpoint (beast::IP::Endpoint const& address) = 0;
    virtual Consumer newInboundEndpoint (beast::IP::Endpoint const& address,
        bool const proxy, boost::string_view const& forwardedFor) = 0;

    
    virtual Consumer newOutboundEndpoint (beast::IP::Endpoint const& address) = 0;

    
    virtual Consumer newUnlimitedEndpoint (
        beast::IP::Endpoint const& address) = 0;

    
    virtual Gossip exportConsumers () = 0;

    
    virtual Json::Value getJson () = 0;
    virtual Json::Value getJson (int threshold) = 0;

    
    virtual void importConsumers (std::string const& origin, Gossip const& gossip) = 0;
};


std::unique_ptr <Manager> make_Manager (
    beast::insight::Collector::ptr const& collector,
        beast::Journal journal);

}
}

#endif








