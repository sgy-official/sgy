

#include <ripple/app/main/Application.h>
#include <ripple/core/Config.h>
#include <ripple/net/RPCErr.h>
#include <ripple/overlay/Overlay.h>
#include <ripple/protocol/ErrorCodes.h>
#include <ripple/protocol/jss.h>
#include <ripple/rpc/Context.h>
#include <ripple/rpc/impl/Handler.h>
#include <ripple/basics/make_lock.h>

namespace ripple {

Json::Value doConnect (RPC::Context& context)
{
    if (context.app.config().standalone())
        return "cannot connect in standalone mode";

    if (!context.params.isMember (jss::ip))
        return RPC::missing_field_error (jss::ip);

    if (context.params.isMember (jss::port) &&
        !context.params[jss::port].isConvertibleTo (Json::intValue))
    {
        return rpcError (rpcINVALID_PARAMS);
    }

    int iPort;

    if(context.params.isMember (jss::port))
        iPort = context.params[jss::port].asInt ();
    else
        iPort = 6561;

    auto ip = beast::IP::Endpoint::from_string(
        context.params[jss::ip].asString ());

    if (! is_unspecified (ip))
        context.app.overlay ().connect (ip.at_port(iPort));

    return RPC::makeObjectValue ("connecting");
}

} 
























