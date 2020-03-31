

#include <ripple/app/ledger/LedgerMaster.h>
#include <ripple/app/misc/NetworkOPs.h>
#include <ripple/json/json_value.h>
#include <ripple/net/RPCErr.h>
#include <ripple/protocol/jss.h>
#include <ripple/rpc/Context.h>

namespace ripple {

Json::Value doLedgerCurrent (RPC::Context& context)
{
    Json::Value jvResult;
    jvResult[jss::ledger_current_index] =
        context.ledgerMaster.getCurrentLedgerIndex ();
    return jvResult;
}

} 
























