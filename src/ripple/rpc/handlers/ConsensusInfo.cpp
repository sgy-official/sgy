

#include <ripple/app/main/Application.h>
#include <ripple/app/misc/NetworkOPs.h>
#include <ripple/json/json_value.h>
#include <ripple/protocol/jss.h>
#include <ripple/rpc/Context.h>
#include <ripple/basics/make_lock.h>

namespace ripple {

Json::Value doConsensusInfo (RPC::Context& context)
{
    Json::Value ret (Json::objectValue);

    ret[jss::info] = context.netOps.getConsensusInfo ();

    return ret;
}

} 
























