

#include <ripple/app/ledger/LedgerMaster.h>
#include <ripple/protocol/ErrorCodes.h>
#include <ripple/protocol/Feature.h>
#include <ripple/resource/Fees.h>
#include <ripple/rpc/Context.h>
#include <ripple/rpc/impl/TransactionSign.h>

namespace ripple {

Json::Value doSubmitMultiSigned (RPC::Context& context)
{
    if (! context.app.getLedgerMaster().getValidatedRules().
        enabled (featureMultiSign))
    {
        RPC::inject_error (rpcNOT_ENABLED, context.params);
        return context.params;
    }
    context.loadType = Resource::feeHighBurdenRPC;
    auto const failHard = context.params[jss::fail_hard].asBool();
    auto const failType = NetworkOPs::doFailHard (failHard);

    return RPC::transactionSubmitMultiSigned (
        context.params,
        failType,
        context.role,
        context.ledgerMaster.getValidatedLedgerAge(),
        context.app,
        RPC::getProcessTxnFn (context.netOps));
}

} 
























