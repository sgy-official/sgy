

#include <ripple/app/main/Application.h>
#include <ripple/basics/Log.h>
#include <ripple/json/json_value.h>
#include <ripple/net/RPCErr.h>
#include <ripple/protocol/ErrorCodes.h>
#include <ripple/protocol/jss.h>
#include <ripple/rpc/Context.h>
#include <boost/algorithm/string/predicate.hpp>

namespace ripple {

Json::Value doLogLevel (RPC::Context& context)
{
    if (!context.params.isMember (jss::severity))
    {
        Json::Value ret (Json::objectValue);
        Json::Value lev (Json::objectValue);

        lev[jss::base] =
            Logs::toString(Logs::fromSeverity(context.app.logs().threshold()));
        std::vector< std::pair<std::string, std::string> > logTable (
            context.app.logs().partition_severities());
        using stringPair = std::map<std::string, std::string>::value_type;
        for (auto const& it : logTable)
            lev[it.first] = it.second;

        ret[jss::levels] = lev;
        return ret;
    }

    LogSeverity const sv (
        Logs::fromString (context.params[jss::severity].asString ()));

    if (sv == lsINVALID)
        return rpcError (rpcINVALID_PARAMS);

    auto severity = Logs::toSeverity(sv);
    if (!context.params.isMember (jss::partition))
    {
        context.app.logs().threshold(severity);
        return Json::objectValue;
    }

    if (context.params.isMember (jss::partition))
    {
        std::string partition (context.params[jss::partition].asString ());

        if (boost::iequals (partition, "base"))
            context.app.logs().threshold (severity);
        else
            context.app.logs().get(partition).threshold(severity);

        return Json::objectValue;
    }

    return rpcError (rpcINVALID_PARAMS);
}

} 
























