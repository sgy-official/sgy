

#include <ripple/app/ledger/LedgerMaster.h>
#include <ripple/app/misc/NetworkOPs.h>
#include <ripple/app/main/Application.h>
#include <ripple/core/Config.h>
#include <ripple/net/RPCErr.h>
#include <ripple/protocol/ErrorCodes.h>
#include <ripple/protocol/Indexes.h>
#include <ripple/protocol/jss.h>
#include <ripple/rpc/Context.h>
#include <ripple/basics/make_lock.h>

namespace ripple {

Json::Value doLedgerAccept (RPC::Context& context)
{
    auto lock = make_lock(context.app.getMasterMutex());
    Json::Value jvResult;

    if (!context.app.config().standalone())
    {
        jvResult[jss::error] = "notStandAlone";
    }
    else
    {
        context.netOps.acceptLedger ();

        jvResult[jss::ledger_current_index] =
            context.ledgerMaster.getCurrentLedgerIndex ();
    }

    return jvResult;
}

} 
























