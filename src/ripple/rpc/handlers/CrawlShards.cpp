

#include <ripple/app/main/Application.h>
#include <ripple/net/RPCErr.h>
#include <ripple/nodestore/DatabaseShard.h>
#include <ripple/overlay/Overlay.h>
#include <ripple/protocol/ErrorCodes.h>
#include <ripple/protocol/jss.h>
#include <ripple/resource/Fees.h>
#include <ripple/rpc/Context.h>

namespace ripple {


Json::Value
doCrawlShards(RPC::Context& context)
{
    if (context.role != Role::ADMIN)
        return rpcError(rpcNO_PERMISSION);

    std::uint32_t hops {0};
    if (auto const& jv = context.params[jss::limit])
    {
        if (!(jv.isUInt() || (jv.isInt() && jv.asInt() >= 0)))
        {
            return RPC::expected_field_error(
                jss::limit, "unsigned integer");
        }

        hops = std::min(jv.asUInt(), csHopLimit);
    }

    bool const pubKey {context.params.isMember(jss::public_key) &&
        context.params[jss::public_key].asBool()};

    Json::Value jvResult {context.app.overlay().crawlShards(pubKey, hops)};

    if (auto shardStore = context.app.getShardStore())
    {
        if (pubKey)
            jvResult[jss::public_key] = toBase58(
                TokenType::NodePublic, context.app.nodeIdentity().first);
        jvResult[jss::complete_shards] = shardStore->getCompleteShards();
    }

    if (hops == 0)
        context.loadType = Resource::feeMediumBurdenRPC;
    else
        context.loadType = Resource::feeHighBurdenRPC;

    return jvResult;
}

} 
























