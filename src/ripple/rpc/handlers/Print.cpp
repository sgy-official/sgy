

#include <ripple/app/main/Application.h>
#include <ripple/json/JsonPropertyStream.h>
#include <ripple/json/json_value.h>
#include <ripple/protocol/jss.h>
#include <ripple/rpc/Context.h>

namespace ripple {

Json::Value doPrint (RPC::Context& context)
{
    JsonPropertyStream stream;
    if (context.params.isObject()
        && context.params[jss::params].isArray()
        && context.params[jss::params][0u].isString ())
    {
        context.app.write (stream, context.params[jss::params][0u].asString());
    }
    else
    {
        context.app.write (stream);
    }

    return stream.top();
}

} 
























