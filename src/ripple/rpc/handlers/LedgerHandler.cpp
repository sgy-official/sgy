

#include <ripple/rpc/handlers/LedgerHandler.h>
#include <ripple/app/ledger/LedgerToJson.h>
#include <ripple/app/main/Application.h>
#include <ripple/app/misc/LoadFeeTrack.h>
#include <ripple/json/Object.h>
#include <ripple/protocol/ErrorCodes.h>
#include <ripple/protocol/jss.h>
#include <ripple/resource/Fees.h>
#include <ripple/rpc/impl/RPCHelpers.h>
#include <ripple/rpc/Role.h>

namespace ripple {
namespace RPC {

LedgerHandler::LedgerHandler (Context& context) : context_ (context)
{
}

Status LedgerHandler::check()
{
    auto const& params = context_.params;
    bool needsLedger = params.isMember (jss::ledger) ||
            params.isMember (jss::ledger_hash) ||
            params.isMember (jss::ledger_index);
    if (! needsLedger)
        return Status::OK;

    if (auto s = lookupLedger (ledger_, context_, result_))
        return s;

    bool const full = params[jss::full].asBool();
    bool const transactions = params[jss::transactions].asBool();
    bool const accounts = params[jss::accounts].asBool();
    bool const expand = params[jss::expand].asBool();
    bool const binary = params[jss::binary].asBool();
    bool const owner_funds = params[jss::owner_funds].asBool();
    bool const queue = params[jss::queue].asBool();
    auto type = chooseLedgerEntryType(params);
    if (type.first)
        return type.first;
    type_ = type.second;

    options_ = (full ? LedgerFill::full : 0)
            | (expand ? LedgerFill::expand : 0)
            | (transactions ? LedgerFill::dumpTxrp : 0)
            | (accounts ? LedgerFill::dumpState : 0)
            | (binary ? LedgerFill::binary : 0)
            | (owner_funds ? LedgerFill::ownerFunds : 0)
            | (queue ? LedgerFill::dumpQueue : 0);

    if (full || accounts)
    {
        if (! isUnlimited (context_.role))
            return rpcNO_PERMISSION;

        if (context_.app.getFeeTrack().isLoadedLocal() &&
            ! isUnlimited (context_.role))
        {
            return rpcTOO_BUSY;
        }
        context_.loadType = binary ? Resource::feeMediumBurdenRPC :
            Resource::feeHighBurdenRPC;
    }
    if (queue)
    {
        if (! ledger_ || ! ledger_->open())
        {
            return rpcINVALID_PARAMS;
        }

        queueTxs_ = context_.app.getTxQ().getTxs(*ledger_);
    }

    return Status::OK;
}

} 
} 
























