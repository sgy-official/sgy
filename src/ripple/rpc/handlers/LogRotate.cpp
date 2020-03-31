

#include <ripple/app/main/Application.h>
#include <ripple/basics/Log.h>
#include <ripple/basics/PerfLog.h>
#include <ripple/rpc/impl/Handler.h>

namespace ripple {

Json::Value doLogRotate (RPC::Context& context)
{
    context.app.getPerfLog().rotate();
    return RPC::makeObjectValue (context.app.logs().rotate());
}

} 
























