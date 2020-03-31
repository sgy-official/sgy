

#include <ripple/net/RPCErr.h>
#include <ripple/protocol/ErrorCodes.h>

namespace ripple {

struct RPCErr;

Json::Value rpcError (int iError, Json::Value jvResult)
{
    RPC::inject_error (iError, jvResult);
    return jvResult;
}

bool isRpcError (Json::Value jvResult)
{
    return jvResult.isObject() && jvResult.isMember (jss::error);
}

} 
























