

#include <ripple/app/main/Application.h>
#include <ripple/basics/BasicConfig.h>
#include <ripple/net/RPCErr.h>
#include <ripple/nodestore/DatabaseShard.h>
#include <ripple/protocol/ErrorCodes.h>
#include <ripple/protocol/jss.h>
#include <ripple/rpc/Context.h>
#include <ripple/rpc/impl/Handler.h>
#include <ripple/rpc/ShardArchiveHandler.h>

#include <boost/algorithm/string.hpp>

namespace ripple {


Json::Value
doDownloadShard(RPC::Context& context)
{
    if (context.role != Role::ADMIN)
        return rpcError(rpcNO_PERMISSION);

    auto shardStore {context.app.getShardStore()};
    if (!shardStore)
        return rpcError(rpcNOT_ENABLED);

    auto preShards {shardStore->getPreShards()};
    if (!preShards.empty())
    {
        std::string s {"Download in progress. Shard"};
        if (!std::all_of(preShards.begin(), preShards.end(), ::isdigit))
            s += "s";
        return RPC::makeObjectValue(s + " " + preShards);
    }

    if (!context.params.isMember(jss::shards))
        return RPC::missing_field_error(jss::shards);
    if (!context.params[jss::shards].isArray() ||
        context.params[jss::shards].size() == 0)
    {
        return RPC::expected_field_error(
            std::string(jss::shards), "an array");
    }

    static const std::string ext {".tar.lz4"};
    std::map<std::uint32_t, parsedURL> archives;
    for (auto& it : context.params[jss::shards])
    {
        if (!it.isMember(jss::index))
            return RPC::missing_field_error(jss::index);
        auto& jv {it[jss::index]};
        if (!(jv.isUInt() || (jv.isInt() && jv.asInt() >= 0)))
        {
            return RPC::expected_field_error(
                std::string(jss::index), "an unsigned integer");
        }

        if (!it.isMember(jss::url))
            return RPC::missing_field_error(jss::url);
        parsedURL url;
        if (!parseUrl(url, it[jss::url].asString()) ||
            url.domain.empty() || url.path.empty())
        {
            return RPC::invalid_field_error(jss::url);
        }
        if (url.scheme != "https")
            return RPC::expected_field_error(std::string(jss::url), "HTTPS");

        auto archiveName {url.path.substr(url.path.find_last_of("/\\") + 1)};
        if (archiveName.empty() || archiveName.size() <= ext.size())
        {
            return RPC::make_param_error("Invalid field '" +
                std::string(jss::url) + "', invalid archive name");
        }
        if (!boost::iends_with(archiveName, ext))
        {
            return RPC::make_param_error("Invalid field '" +
                std::string(jss::url) + "', invalid archive extension");
        }

        if (!archives.emplace(jv.asUInt(), std::move(url)).second)
        {
            return RPC::make_param_error("Invalid field '" +
                std::string(jss::index) + "', duplicate shard ids.");
        }
    }

    bool validate {true};
    if (context.params.isMember(jss::validate))
    {
        if (!context.params[jss::validate].isBool())
        {
            return RPC::expected_field_error(
                std::string(jss::validate), "a bool");
        }
        validate = context.params[jss::validate].asBool();
    }

    auto handler {
        std::make_shared<RPC::ShardArchiveHandler>(context.app, validate)};
    for (auto& ar : archives)
    {
        if (!handler->add(ar.first, std::move(ar.second)))
        {
            return RPC::make_param_error("Invalid field '" +
                std::string(jss::index) + "', shard id " +
                std::to_string(ar.first) + " exists or being acquired");
        }
    }
    if (!handler->start())
        return rpcError(rpcINTERNAL);

    std::string s {"Downloading shard"};
    preShards = shardStore->getPreShards();
    if (!std::all_of(preShards.begin(), preShards.end(), ::isdigit))
        s += "s";
    return RPC::makeObjectValue(s + " " + preShards);
}

} 
























