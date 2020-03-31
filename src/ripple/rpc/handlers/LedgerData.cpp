

#include <ripple/app/ledger/LedgerToJson.h>
#include <ripple/ledger/ReadView.h>
#include <ripple/protocol/ErrorCodes.h>
#include <ripple/protocol/jss.h>
#include <ripple/protocol/LedgerFormats.h>
#include <ripple/rpc/impl/RPCHelpers.h>
#include <ripple/rpc/impl/Tuning.h>
#include <ripple/rpc/Context.h>
#include <ripple/rpc/Role.h>

namespace ripple {

Json::Value doLedgerData (RPC::Context& context)
{
    std::shared_ptr<ReadView const> lpLedger;
    auto const& params = context.params;

    auto jvResult = RPC::lookupLedger(lpLedger, context);
    if (!lpLedger)
        return jvResult;

    bool const isMarker = params.isMember (jss::marker);
    ReadView::key_type key = ReadView::key_type();
    if (isMarker)
    {
        Json::Value const& jMarker = params[jss::marker];
        if (! (jMarker.isString () && key.SetHex (jMarker.asString ())))
            return RPC::expected_field_error (jss::marker, "valid");
    }

    bool const isBinary = params[jss::binary].asBool();

    int limit = -1;
    if (params.isMember (jss::limit))
    {
        Json::Value const& jLimit = params[jss::limit];
        if (!jLimit.isIntegral ())
            return RPC::expected_field_error (jss::limit, "integer");

        limit = jLimit.asInt ();
    }

    auto maxLimit = RPC::Tuning::pageLength(isBinary);
    if ((limit < 0) || ((limit > maxLimit) && (! isUnlimited (context.role))))
        limit = maxLimit;

    jvResult[jss::ledger_hash] = to_string (lpLedger->info().hash);
    jvResult[jss::ledger_index] = lpLedger->info().seq;

    if (! isMarker)
    {
        jvResult[jss::ledger] = getJson (
            LedgerFill (*lpLedger, isBinary ?
                LedgerFill::Options::binary : 0));
    }

    auto type = RPC::chooseLedgerEntryType(params);
    if (type.first)
    {
        jvResult.clear();
        type.first.inject(jvResult);
        return jvResult;
    }
    Json::Value& nodes = jvResult[jss::state];

    auto e = lpLedger->sles.end();
    for (auto i = lpLedger->sles.upper_bound(key); i != e; ++i)
    {
        auto sle = lpLedger->read(keylet::unchecked((*i)->key()));
        if (limit-- <= 0)
        {
            auto k = sle->key();
            jvResult[jss::marker] = to_string(--k);
            break;
        }

        if (type.second == ltINVALID || sle->getType () == type.second)
        {
            if (isBinary)
            {
                Json::Value& entry = nodes.append (Json::objectValue);
                entry[jss::data] = serializeHex(*sle);
                entry[jss::index] = to_string(sle->key());
            }
            else
            {
                Json::Value& entry = nodes.append (sle->getJson (JsonOptions::none));
                entry[jss::index] = to_string(sle->key());
            }
        }
    }

    return jvResult;
}

} 
























