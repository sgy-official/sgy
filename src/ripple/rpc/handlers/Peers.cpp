

#include <ripple/app/main/Application.h>
#include <ripple/app/misc/LoadFeeTrack.h>
#include <ripple/core/TimeKeeper.h>
#include <ripple/overlay/Cluster.h>
#include <ripple/overlay/Overlay.h>
#include <ripple/protocol/jss.h>
#include <ripple/rpc/Context.h>
#include <ripple/basics/make_lock.h>

namespace ripple {

Json::Value doPeers (RPC::Context& context)
{
    Json::Value jvResult (Json::objectValue);

    {
        jvResult[jss::peers] = context.app.overlay ().json ();

        auto const now = context.app.timeKeeper().now();
        auto const self = context.app.nodeIdentity().first;

        Json::Value& cluster = (jvResult[jss::cluster] = Json::objectValue);
        std::uint32_t ref = context.app.getFeeTrack().getLoadBase();

        context.app.cluster().for_each ([&cluster, now, ref, &self]
            (ClusterNode const& node)
            {
                if (node.identity() == self)
                    return;

                Json::Value& json = cluster[
                    toBase58(
                        TokenType::NodePublic,
                        node.identity())];

                if (!node.name().empty())
                    json[jss::tag] = node.name();

                if ((node.getLoadFee() != ref) && (node.getLoadFee() != 0))
                    json[jss::fee] = static_cast<double>(node.getLoadFee()) / ref;

                if (node.getReportTime() != NetClock::time_point{})
                    json[jss::age] = (node.getReportTime() >= now)
                        ? 0
                        : (now - node.getReportTime()).count();
            });
    }

    return jvResult;
}

} 
























