

#include <ripple/app/ledger/LedgerMaster.h>
#include <ripple/app/main/Application.h>
#include <ripple/app/misc/NetworkOPs.h>
#include <ripple/app/misc/Transaction.h>
#include <ripple/ledger/ReadView.h>
#include <ripple/net/RPCErr.h>
#include <ripple/protocol/ErrorCodes.h>
#include <ripple/protocol/jss.h>
#include <ripple/resource/Fees.h>
#include <ripple/rpc/Context.h>
#include <ripple/rpc/DeliveredAmount.h>
#include <ripple/rpc/impl/RPCHelpers.h>
#include <ripple/rpc/Role.h>

namespace ripple {

Json::Value doAccountTxOld (RPC::Context& context)
{
    std::uint32_t offset
            = context.params.isMember (jss::offset)
            ? context.params[jss::offset].asUInt () : 0;
    int limit = context.params.isMember (jss::limit)
            ? context.params[jss::limit].asUInt () : -1;
    bool bBinary = context.params.isMember (jss::binary)
            && context.params[jss::binary].asBool ();
    bool bDescending = context.params.isMember (jss::descending)
            && context.params[jss::descending].asBool ();
    bool bCount = context.params.isMember (jss::count)
            && context.params[jss::count].asBool ();
    std::uint32_t   uLedgerMin;
    std::uint32_t   uLedgerMax;
    std::uint32_t   uValidatedMin;
    std::uint32_t   uValidatedMax;
    bool bValidated  = context.ledgerMaster.getValidatedRange (
        uValidatedMin, uValidatedMax);

    if (!context.params.isMember (jss::account))
        return rpcError (rpcINVALID_PARAMS);

    auto const raAccount = parseBase58<AccountID>(
        context.params[jss::account].asString());
    if (! raAccount)
        return rpcError (rpcACT_MALFORMED);

    if (offset > 3000)
        return rpcError (rpcATX_DEPRECATED);

    context.loadType = Resource::feeHighBurdenRPC;

    if (context.params.isMember (jss::ledger_min))
    {
        context.params[jss::ledger_index_min]   = context.params[jss::ledger_min];
        bDescending = true;
    }

    if (context.params.isMember (jss::ledger_max))
    {
        context.params[jss::ledger_index_max]   = context.params[jss::ledger_max];
        bDescending = true;
    }

    if (context.params.isMember (jss::ledger_index_min)
        || context.params.isMember (jss::ledger_index_max))
    {
        std::int64_t iLedgerMin  = context.params.isMember (jss::ledger_index_min)
                ? context.params[jss::ledger_index_min].asInt () : -1;
        std::int64_t iLedgerMax  = context.params.isMember (jss::ledger_index_max)
                ? context.params[jss::ledger_index_max].asInt () : -1;

        if (!bValidated && (iLedgerMin == -1 || iLedgerMax == -1))
        {
            return rpcError (rpcLGR_IDXS_INVALID);
        }

        uLedgerMin  = iLedgerMin == -1 ? uValidatedMin : iLedgerMin;
        uLedgerMax  = iLedgerMax == -1 ? uValidatedMax : iLedgerMax;

        if (uLedgerMax < uLedgerMin)
        {
            return rpcError (rpcLGR_IDXS_INVALID);
        }
    }
    else
    {
        std::shared_ptr<ReadView const> ledger;
        auto ret = RPC::lookupLedger (ledger, context);

        if (!ledger)
            return ret;

        if (! ret[jss::validated].asBool() ||
            (ledger->info().seq > uValidatedMax) ||
            (ledger->info().seq < uValidatedMin))
        {
            return rpcError (rpcLGR_NOT_VALIDATED);
        }

        uLedgerMin = uLedgerMax = ledger->info().seq;
    }

    int count = 0;

#ifndef BEAST_DEBUG

    try
    {
#endif

        Json::Value ret (Json::objectValue);

        ret[jss::account] = context.app.accountIDCache().toBase58(*raAccount);
        Json::Value& jvTxns = (ret[jss::transactions] = Json::arrayValue);

        if (bBinary)
        {
            auto txns = context.netOps.getAccountTxsB (
                *raAccount, uLedgerMin, uLedgerMax, bDescending, offset, limit,
                isUnlimited (context.role));

            for (auto it = txns.begin (), end = txns.end (); it != end; ++it)
            {
                ++count;
                Json::Value& jvObj = jvTxns.append (Json::objectValue);

                std::uint32_t  uLedgerIndex = std::get<2> (*it);
                jvObj[jss::tx_blob]            = std::get<0> (*it);
                jvObj[jss::meta]               = std::get<1> (*it);
                jvObj[jss::ledger_index]       = uLedgerIndex;
                jvObj[jss::validated]
                        = bValidated
                        && uValidatedMin <= uLedgerIndex
                        && uValidatedMax >= uLedgerIndex;

            }
        }
        else
        {
            auto txns = context.netOps.getAccountTxs (
                *raAccount, uLedgerMin, uLedgerMax, bDescending, offset, limit,
                isUnlimited (context.role));

            for (auto it = txns.begin (), end = txns.end (); it != end; ++it)
            {
                ++count;
                Json::Value&    jvObj = jvTxns.append (Json::objectValue);

                if (it->first)
                    jvObj[jss::tx] =
                        it->first->getJson (JsonOptions::include_date);

                if (it->second)
                {
                    std::uint32_t uLedgerIndex = it->second->getLgrSeq ();

                    auto meta = it->second->getJson(JsonOptions::none);
                    insertDeliveredAmount(meta, context, it->first, *it->second);
                    jvObj[jss::meta] = std::move(meta);

                    jvObj[jss::validated]
                            = bValidated
                            && uValidatedMin <= uLedgerIndex
                            && uValidatedMax >= uLedgerIndex;
                }

            }
        }

        ret[jss::ledger_index_min] = uLedgerMin;
        ret[jss::ledger_index_max] = uLedgerMax;
        ret[jss::validated]
                = bValidated
                && uValidatedMin <= uLedgerMin
                && uValidatedMax >= uLedgerMax;
        ret[jss::offset]           = offset;

        if (bCount)
            ret[jss::count]        = count;

        if (context.params.isMember (jss::limit))
            ret[jss::limit]        = limit;


        return ret;
#ifndef BEAST_DEBUG
    }
    catch (std::exception const&)
    {
        return rpcError (rpcINTERNAL);
    }

#endif
}

} 
























