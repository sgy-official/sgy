

#include <ripple/app/ledger/LedgerMaster.h>
#include <ripple/app/paths/PathRequests.h>
#include <ripple/net/RPCErr.h>
#include <ripple/resource/Fees.h>
#include <ripple/rpc/Context.h>
#include <ripple/rpc/impl/LegacyPathFind.h>
#include <ripple/rpc/impl/RPCHelpers.h>

namespace ripple {

Json::Value doRipplePathFind (RPC::Context& context)
{
    if (context.app.config().PATH_SEARCH_MAX == 0)
        return rpcError (rpcNOT_SUPPORTED);

    context.loadType = Resource::feeHighBurdenRPC;

    std::shared_ptr <ReadView const> lpLedger;
    Json::Value jvResult;

    if (! context.app.config().standalone() &&
        ! context.params.isMember(jss::ledger) &&
        ! context.params.isMember(jss::ledger_index) &&
        ! context.params.isMember(jss::ledger_hash))
    {
        if (context.app.getLedgerMaster().getValidatedLedgerAge() >
            RPC::Tuning::maxValidatedLedgerAge)
        {
            return rpcError (rpcNO_NETWORK);
        }

        PathRequest::pointer request;
        lpLedger = context.ledgerMaster.getClosedLedger();

        jvResult = context.app.getPathRequests().makeLegacyPathRequest (
            request,
            [&context] () {
                std::shared_ptr<JobQueue::Coro> coroCopy {context.coro};
                if (!coroCopy->post())
                {
                    coroCopy->resume();
                }
            },
            context.consumer, lpLedger, context.params);
        if (request)
        {
            context.coro->yield();
            jvResult = request->doStatus (context.params);
        }

        return jvResult;
    }

    jvResult = RPC::lookupLedger (lpLedger, context);
    if (! lpLedger)
        return jvResult;

    RPC::LegacyPathFind lpf (isUnlimited (context.role), context.app);
    if (! lpf.isOk ())
        return rpcError (rpcTOO_BUSY);

    auto result = context.app.getPathRequests().doLegacyPathRequest (
        context.consumer, lpLedger, context.params);

    for (auto &fieldName : jvResult.getMemberNames ())
        result[fieldName] = std::move (jvResult[fieldName]);

    return result;
}

} 
























