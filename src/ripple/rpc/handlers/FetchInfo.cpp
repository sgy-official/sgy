

#include <ripple/app/misc/NetworkOPs.h>
#include <ripple/json/json_value.h>
#include <ripple/protocol/jss.h>
#include <ripple/rpc/Context.h>

namespace ripple {

Json::Value doFetchInfo (RPC::Context& context)
{
    Json::Value ret (Json::objectValue);

    if (context.params.isMember(jss::clear) && context.params[jss::clear].asBool())
    {
        context.netOps.clearLedgerFetch();
        ret[jss::clear] = true;
    }

    ret[jss::info] = context.netOps.getLedgerFetchInfo();

    return ret;
}

} 
























