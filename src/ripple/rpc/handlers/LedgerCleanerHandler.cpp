

#include <ripple/app/ledger/LedgerMaster.h>
#include <ripple/app/main/Application.h>
#include <ripple/json/json_value.h>
#include <ripple/rpc/Context.h>
#include <ripple/rpc/impl/Handler.h>

namespace ripple {

Json::Value doLedgerCleaner (RPC::Context& context)
{
    context.app.getLedgerMaster().doLedgerCleaner (context.params);
    return RPC::makeObjectValue ("Cleaner configured");
}


} 
























