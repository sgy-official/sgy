

#ifndef RIPPLE_SERVER_ROLE_H_INCLUDED
#define RIPPLE_SERVER_ROLE_H_INCLUDED

#include <ripple/beast/net/IPEndpoint.h>
#include <ripple/json/json_value.h>
#include <ripple/resource/ResourceManager.h>
#include <ripple/server/Handoff.h>
#include <ripple/server/Port.h>
#include <boost/utility/string_view.hpp>
#include <string>
#include <vector>

namespace ripple {


enum class Role
{
    GUEST,
    USER,
    IDENTIFIED,
    ADMIN,
    PROXY,
    FORBID
};


Role
requestRole (Role const& required, Port const& port,
    Json::Value const& params, beast::IP::Endpoint const& remoteIp,
    boost::string_view const& user);

Resource::Consumer
requestInboundEndpoint (Resource::Manager& manager,
    beast::IP::Endpoint const& remoteAddress, Role const& role,
    boost::string_view const& user, boost::string_view const& forwardedFor);


bool
isUnlimited (Role const& role);


bool
ipAllowed (beast::IP::Address const& remoteIp,
    std::vector<beast::IP::Address> const& adminIp);

boost::string_view
forwardedFor(http_request_type const& request);

} 

#endif








