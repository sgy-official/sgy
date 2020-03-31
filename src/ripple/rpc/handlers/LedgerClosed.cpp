

#include <ripple/app/ledger/LedgerMaster.h>
#include <ripple/app/misc/NetworkOPs.h>
#include <ripple/json/json_value.h>
#include <ripple/protocol/jss.h>
#include <ripple/rpc/Context.h>

namespace ripple {

Json::Value doLedgerClosed (RPC::Context& context)
{
    auto ledger = context.ledgerMaster.getClosedLedger ();
    assert (ledger);

    Json::Value jvResult;
    jvResult[jss::ledger_index] = ledger->info().seq;
    jvResult[jss::ledger_hash] = to_string (ledger->info().hash);

    return jvResult;
}

} 
























