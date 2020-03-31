

#include <ripple/app/ledger/LedgerMaster.h>
#include <ripple/protocol/ErrorCodes.h>
#include <ripple/resource/Fees.h>
#include <ripple/rpc/Context.h>
#include <ripple/rpc/impl/TransactionSign.h>

namespace ripple {

Json::Value doSign (RPC::Context& context)
{
    if (context.role != Role::ADMIN && !context.app.config().canSign())
    {
        return RPC::make_error (rpcNOT_SUPPORTED,
            "Signing is not supported by this server.");
    }

    context.loadType = Resource::feeHighBurdenRPC;
    NetworkOPs::FailHard const failType =
        NetworkOPs::doFailHard (
            context.params.isMember (jss::fail_hard)
            && context.params[jss::fail_hard].asBool ());

    auto ret = RPC::transactionSign (
        context.params, failType, context.role,
        context.ledgerMaster.getValidatedLedgerAge(), context.app);

    ret[jss::deprecated] = "This command has been deprecated and will be "
                           "removed in a future version of the server. Please "
                           "migrate to a standalone signing tool.";

    return ret;
}

} 
























