

#include <ripple/json/json_value.h>
#include <ripple/net/RPCErr.h>
#include <ripple/protocol/jss.h>
#include <ripple/resource/Fees.h>
#include <ripple/rpc/Context.h>

namespace ripple {

Json::Value doAccountTxOld (RPC::Context& context);
        Json::Value doAccountTx (RPC::Context& context);

Json::Value doAccountTxSwitch (RPC::Context& context)
{
    if (context.params.isMember(jss::offset) ||
        context.params.isMember(jss::count) ||
        context.params.isMember(jss::descending) ||
        context.params.isMember(jss::ledger_max) ||
        context.params.isMember(jss::ledger_min))
    {
        return doAccountTxOld(context);
    }
    return doAccountTx(context);
}

} 
























