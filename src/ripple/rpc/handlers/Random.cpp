

#include <ripple/crypto/csprng.h>
#include <ripple/json/json_value.h>
#include <ripple/net/RPCErr.h>
#include <ripple/protocol/ErrorCodes.h>
#include <ripple/protocol/jss.h>
#include <ripple/basics/base_uint.h>
#include <ripple/beast/utility/rngfill.h>

namespace ripple {

namespace RPC {
struct Context;
}

Json::Value doRandom (RPC::Context& context)
{
    try
    {
        uint256 rand;
        beast::rngfill (
            rand.begin(),
            rand.size(),
            crypto_prng());

        Json::Value jvResult;
        jvResult[jss::random]  = to_string (rand);
        return jvResult;
    }
    catch (std::exception const&)
    {
        return rpcError (rpcINTERNAL);
    }
}

} 
























