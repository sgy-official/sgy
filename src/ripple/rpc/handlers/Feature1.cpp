

#include <ripple/app/ledger/LedgerMaster.h>
#include <ripple/app/main/Application.h>
#include <ripple/app/misc/AmendmentTable.h>
#include <ripple/protocol/ErrorCodes.h>
#include <ripple/protocol/jss.h>
#include <ripple/net/RPCErr.h>
#include <ripple/rpc/Context.h>
#include <ripple/beast/core/LexicalCast.h>

namespace ripple {


Json::Value doFeature (RPC::Context& context)
{

    majorityAmendments_t majorities;

    if (auto const valLedger = context.ledgerMaster.getValidatedLedger())
        majorities = getMajorityAmendments (*valLedger);

    auto& table = context.app.getAmendmentTable ();

    if (!context.params.isMember (jss::feature))
    {
        auto features = table.getJson(0);

        for (auto const& m : majorities)
        {
            features[to_string(m.first)][jss::majority] =
                m.second.time_since_epoch().count();
        }

        Json::Value jvReply = Json::objectValue;
        jvReply[jss::features] = features;
        return jvReply;
    }

    auto feature = table.find (
        context.params[jss::feature].asString());

    if (!feature &&
        !feature.SetHexExact (context.params[jss::feature].asString ()))
        return rpcError (rpcBAD_FEATURE);

    if (context.params.isMember (jss::vetoed))
    {
        if (context.params[jss::vetoed].asBool ())
            context.app.getAmendmentTable().veto (feature);
        else
            context.app.getAmendmentTable().unVeto(feature);
    }

    Json::Value jvReply = table.getJson(feature);

    auto m = majorities.find (feature);
    if (m != majorities.end())
        jvReply [jss::majority] =
            m->second.time_since_epoch().count();

    return jvReply;
}


} 
























