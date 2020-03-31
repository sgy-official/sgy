

#include <ripple/app/ledger/LedgerMaster.h>
#include <ripple/protocol/ErrorCodes.h>
#include <ripple/protocol/Feature.h>
#include <ripple/resource/Fees.h>
#include <ripple/rpc/Context.h>
#include <ripple/rpc/impl/TransactionSign.h>

namespace ripple {

Json::Value doSignFor (RPC::Context& context)
{
    if (context.role != Role::ADMIN && !context.app.config().canSign())
    {
        return RPC::make_error (rpcNOT_SUPPORTED,
            "Signing is not supported by this server.");
    }

    if (! context.app.getLedgerMaster().getValidatedRules().
        enabled (featureMultiSign))
    {
        RPC::inject_error (rpcNOT_ENABLED, context.params);
        return context.params;
    }
    context.loadType = Resource::feeHighBurdenRPC;
    auto const failHard = context.params[jss::fail_hard].asBool();
    auto const failType = NetworkOPs::doFailHard (failHard);

    auto ret = RPC::transactionSignFor (
        context.params, failType, context.role,
        context.ledgerMaster.getValidatedLedgerAge(), context.app);

    ret[jss::deprecated] = "This command has been deprecated and will be "
                           "removed in a future version of the server. Please "
                           "migrate to a standalone signing tool.";
    return ret;
}

} 
























