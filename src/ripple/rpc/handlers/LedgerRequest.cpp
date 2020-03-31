

#include <ripple/app/ledger/InboundLedgers.h>
#include <ripple/app/ledger/LedgerToJson.h>
#include <ripple/app/ledger/LedgerMaster.h>
#include <ripple/app/main/Application.h>
#include <ripple/net/RPCErr.h>
#include <ripple/resource/Fees.h>
#include <ripple/rpc/Context.h>
#include <ripple/protocol/ErrorCodes.h>
#include <ripple/protocol/jss.h>
#include <ripple/rpc/impl/Tuning.h>

namespace ripple {

Json::Value doLedgerRequest (RPC::Context& context)
{
    auto const hasHash = context.params.isMember (jss::ledger_hash);
    auto const hasIndex = context.params.isMember (jss::ledger_index);
    std::uint32_t ledgerIndex = 0;

    auto& ledgerMaster = context.app.getLedgerMaster();
    LedgerHash ledgerHash;

    if ((hasHash && hasIndex) || !(hasHash || hasIndex))
    {
        return RPC::make_param_error(
            "Exactly one of ledger_hash and ledger_index can be set.");
    }

    context.loadType = Resource::feeHighBurdenRPC;

    if (hasHash)
    {
        auto const& jsonHash = context.params[jss::ledger_hash];
        if (!jsonHash.isString() || !ledgerHash.SetHex (jsonHash.asString ()))
            return RPC::invalid_field_error (jss::ledger_hash);
    }
    else
    {
        auto const& jsonIndex = context.params[jss::ledger_index];
        if (!jsonIndex.isInt())
            return RPC::invalid_field_error (jss::ledger_index);

        if (ledgerMaster.getValidatedLedgerAge() >
            RPC::Tuning::maxValidatedLedgerAge)
            return rpcError (rpcNO_CURRENT);

        ledgerIndex = jsonIndex.asInt();
        auto ledger = ledgerMaster.getValidatedLedger();

        if (ledgerIndex >= ledger->info().seq)
            return RPC::make_param_error("Ledger index too large");
        if (ledgerIndex <= 0)
            return RPC::make_param_error("Ledger index too small");

        auto const j = context.app.journal("RPCHandler");
        auto neededHash = hashOfSeq(*ledger, ledgerIndex, j);
        if (! neededHash)
        {
            auto const refIndex = getCandidateLedger(ledgerIndex);
            auto refHash = hashOfSeq(*ledger, refIndex, j);
            assert(refHash);

            ledger = ledgerMaster.getLedgerByHash (*refHash);
            if (! ledger)
            {

                if (auto il = context.app.getInboundLedgers().acquire (
                        *refHash, refIndex, InboundLedger::Reason::GENERIC))
                {
                    Json::Value jvResult = RPC::make_error(
                        rpcLGR_NOT_FOUND,
                            "acquiring ledger containing requested index");
                    jvResult[jss::acquiring] = getJson (LedgerFill (*il));
                    return jvResult;
                }

                if (auto il = context.app.getInboundLedgers().find (*refHash))
                {
                    Json::Value jvResult = RPC::make_error(
                        rpcLGR_NOT_FOUND,
                            "acquiring ledger containing requested index");
                    jvResult[jss::acquiring] = il->getJson (0);
                    return jvResult;
                }

                return Json::Value();
            }

            neededHash = hashOfSeq(*ledger, ledgerIndex, j);
        }
        assert (neededHash);
        ledgerHash = neededHash ? *neededHash : beast::zero; 
    }

    auto ledger = context.app.getInboundLedgers().acquire (
        ledgerHash, ledgerIndex, InboundLedger::Reason::GENERIC);

    if (! ledger && context.app.config().standalone())
        ledger = ledgerMaster.getLedgerByHash (ledgerHash);

    if (ledger)
    {
        Json::Value jvResult;
        jvResult[jss::ledger_index] = ledger->info().seq;
        addJson (jvResult, {*ledger, 0});
        return jvResult;
    }

    if (auto il = context.app.getInboundLedgers().find (ledgerHash))
        return il->getJson (0);

    return RPC::make_error (
        rpcNOT_READY, "findCreate failed to return an inbound ledger");
}

} 
























