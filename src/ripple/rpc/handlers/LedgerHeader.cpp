

#include <ripple/app/ledger/LedgerToJson.h>
#include <ripple/basics/strHex.h>
#include <ripple/ledger/ReadView.h>
#include <ripple/protocol/jss.h>
#include <ripple/rpc/impl/RPCHelpers.h>

namespace ripple {

Json::Value doLedgerHeader (RPC::Context& context)
{
    std::shared_ptr<ReadView const> lpLedger;
    auto jvResult = RPC::lookupLedger (lpLedger, context);

    if (!lpLedger)
        return jvResult;

    Serializer  s;
    addRaw (lpLedger->info(), s);
    jvResult[jss::ledger_data] = strHex (s.peekData ());

    addJson (jvResult, {*lpLedger, 0});

    return jvResult;
}


} 
























