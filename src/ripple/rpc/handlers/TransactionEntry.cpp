

#include <ripple/app/main/Application.h>
#include <ripple/ledger/ReadView.h>
#include <ripple/protocol/jss.h>
#include <ripple/rpc/Context.h>
#include <ripple/rpc/impl/RPCHelpers.h>

namespace ripple {

Json::Value doTransactionEntry (RPC::Context& context)
{
    std::shared_ptr<ReadView const> lpLedger;
    Json::Value jvResult = RPC::lookupLedger (lpLedger, context);

    if(! lpLedger)
        return jvResult;

    if(! context.params.isMember (jss::tx_hash))
    {
        jvResult[jss::error] = "fieldNotFoundTransaction";
    }
    else if(jvResult.get(jss::ledger_hash, Json::nullValue).isNull())
    {

        jvResult[jss::error]   = "notYetImplemented";
    }
    else
    {
        uint256 uTransID;
        uTransID.SetHex (context.params[jss::tx_hash].asString ());

        auto tx = lpLedger->txRead (uTransID);
        if(! tx.first)
        {
            jvResult[jss::error]   = "transactionNotFound";
        }
        else
        {
            jvResult[jss::tx_json] = tx.first->getJson (JsonOptions::none);
            if (tx.second)
                jvResult[jss::metadata] =
                    tx.second->getJson (JsonOptions::none);
        }
    }

    return jvResult;
}

} 
























