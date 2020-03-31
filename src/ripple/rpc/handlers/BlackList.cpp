

#include <ripple/app/main/Application.h>
#include <ripple/protocol/jss.h>
#include <ripple/resource/ResourceManager.h>
#include <ripple/rpc/Context.h>

namespace ripple {

Json::Value doBlackList (RPC::Context& context)
{
    auto& rm = context.app.getResourceManager();
    if (context.params.isMember(jss::threshold))
        return rm.getJson(context.params[jss::threshold].asInt());
    else
        return rm.getJson();
}

} 
























