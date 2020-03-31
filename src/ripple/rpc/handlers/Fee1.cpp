

#include <ripple/app/ledger/OpenLedger.h>
#include <ripple/app/main/Application.h>
#include <ripple/app/misc/TxQ.h>
#include <ripple/rpc/Context.h>
#include <ripple/protocol/ErrorCodes.h>
#include <ripple/protocol/Feature.h>

namespace ripple
{
    Json::Value doFee(RPC::Context& context)
    {
        auto result = context.app.getTxQ().doRPC(context.app);
        if (result.type() == Json::objectValue)
            return result;
        assert(false);
        RPC::inject_error(rpcINTERNAL, context.params);
        return context.params;
    }
} 
























