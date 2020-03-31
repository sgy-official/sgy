

#include <ripple/app/main/Application.h>
#include <ripple/json/json_value.h>
#include <ripple/rpc/impl/Handler.h>
#include <ripple/basics/make_lock.h>

namespace ripple {

namespace RPC {
struct Context;
}

Json::Value doStop (RPC::Context& context)
{
    auto lock = make_lock(context.app.getMasterMutex());
    context.app.signalStop ();

    return RPC::makeObjectValue (systemName () + " server stopping");
}

} 
























