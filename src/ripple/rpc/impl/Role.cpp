

#include <ripple/rpc/Role.h>
#include <boost/beast/core/string.hpp>
#include <algorithm>

namespace ripple {

bool
passwordUnrequiredOrSentCorrect (Port const& port,
                                 Json::Value const& params) {

    assert(! port.admin_ip.empty ());
    bool const passwordRequired = (!port.admin_user.empty() ||
                                   !port.admin_password.empty());

    return !passwordRequired  ||
            ((params["admin_password"].isString() &&
              params["admin_password"].asString() == port.admin_password) &&
             (params["admin_user"].isString() &&
              params["admin_user"].asString() == port.admin_user));
}

bool
ipAllowed (beast::IP::Address const& remoteIp,
           std::vector<beast::IP::Address> const& adminIp)
{
    return std::find_if (adminIp.begin (), adminIp.end (),
        [&remoteIp](beast::IP::Address const& ip) { return ip.is_unspecified () ||
            ip == remoteIp; }) != adminIp.end ();
}

bool
isAdmin (Port const& port, Json::Value const& params,
         beast::IP::Address const& remoteIp)
{
    return ipAllowed (remoteIp, port.admin_ip) &&
        passwordUnrequiredOrSentCorrect (port, params);
}

Role
requestRole (Role const& required, Port const& port,
             Json::Value const& params, beast::IP::Endpoint const& remoteIp,
             boost::string_view const& user)
{
    if (isAdmin(port, params, remoteIp.address()))
        return Role::ADMIN;

    if (required == Role::ADMIN)
        return Role::FORBID;

    if (ipAllowed(remoteIp.address(), port.secure_gateway_ip))
    {
        if (user.size())
            return Role::IDENTIFIED;
        return Role::PROXY;
    }

    return Role::GUEST;
}


bool
isUnlimited (Role const& role)
{
    return role == Role::ADMIN || role == Role::IDENTIFIED;
}

bool
isUnlimited (Role const& required, Port const& port,
    Json::Value const& params, beast::IP::Endpoint const& remoteIp,
    std::string const& user)
{
    return isUnlimited(requestRole(required, port, params, remoteIp, user));
}

Resource::Consumer
requestInboundEndpoint (Resource::Manager& manager,
    beast::IP::Endpoint const& remoteAddress, Role const& role,
    boost::string_view const& user, boost::string_view const& forwardedFor)
{
    if (isUnlimited(role))
        return manager.newUnlimitedEndpoint (remoteAddress);

    return manager.newInboundEndpoint(remoteAddress, role == Role::PROXY,
        forwardedFor);
}

boost::string_view
forwardedFor(http_request_type const& request)
{
    auto it = request.find("X-Forwarded-For");
    if (it != request.end())
    {
        return boost::beast::http::ext_list{
            it->value()}.begin()->first;
    }

    it = request.find("Forwarded");
    if (it != request.end())
    {
        static std::string const forStr{"for="};
        auto found = std::search(it->value().begin(), it->value().end(),
            forStr.begin(), forStr.end(),
            [](char c1, char c2)
            {
                return boost::beast::detail::ascii_tolower(c1) ==
                       boost::beast::detail::ascii_tolower(c2);
            }
        );

        if (found == it->value().end())
            return {};

        found += forStr.size();
        auto pos{it->value().find(';', forStr.size())};
        if (pos != boost::string_view::npos)
            return {found, pos + 1};
        return {found, it->value().size() - forStr.size()};
    }

    return {};
}

}
























