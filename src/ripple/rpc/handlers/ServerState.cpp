

#include <ripple/app/misc/NetworkOPs.h>
#include <ripple/json/json_value.h>
#include <ripple/net/RPCErr.h>
#include <ripple/protocol/jss.h>
#include <ripple/rpc/Context.h>
#include <ripple/rpc/impl/TransactionSign.h>
#include <ripple/rpc/Role.h>

namespace ripple {

Json::Value doServerState (RPC::Context& context)
{
    Json::Value ret (Json::objectValue);

    ret[jss::state] = context.netOps.getServerInfo (
        false, context.role == Role::ADMIN,
        context.params.isMember(jss::counters) &&
            context.params[jss::counters].asBool());

    return ret;
}

} 
























