

#ifndef RIPPLE_RPC_RPCHANDLER_H_INCLUDED
#define RIPPLE_RPC_RPCHANDLER_H_INCLUDED

#include <ripple/core/Config.h>
#include <ripple/net/InfoSub.h>
#include <ripple/rpc/Context.h>
#include <ripple/rpc/Status.h>

namespace ripple {
namespace RPC {

struct Context;


Status doCommand (RPC::Context&, Json::Value&);

Role roleRequired (std::string const& method );

} 
} 

#endif








