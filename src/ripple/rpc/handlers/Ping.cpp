

#include <ripple/json/json_value.h>
#include <ripple/protocol/jss.h>
#include <ripple/rpc/Role.h>
#include <ripple/rpc/Context.h>

namespace ripple {

namespace RPC {
struct Context;
} 

Json::Value doPing (RPC::Context& context)
{
    Json::Value ret(Json::objectValue);
    switch (context.role)
    {
        case Role::ADMIN:
            ret[jss::role] = "admin";
            break;
        case Role::IDENTIFIED:
            ret[jss::role] = "identified";
            ret[jss::username] = context.headers.user.to_string();
            if (context.headers.forwardedFor.size())
                ret[jss::ip] = context.headers.forwardedFor.to_string();
            break;
        case Role::PROXY:
            ret[jss::role] = "proxied";
            ret[jss::ip] = context.headers.forwardedFor.to_string();
        default:
            ;
    }

    if (context.infoSub)
    {
        if (context.infoSub->getConsumer().isUnlimited())
            ret[jss::unlimited] = true;
    }

    return ret;
}

} 
























