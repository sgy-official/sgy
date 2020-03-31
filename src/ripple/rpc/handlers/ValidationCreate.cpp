

#include <ripple/basics/Log.h>
#include <ripple/net/RPCErr.h>
#include <ripple/protocol/ErrorCodes.h>
#include <ripple/protocol/jss.h>
#include <ripple/protocol/Seed.h>
#include <ripple/rpc/Context.h>

namespace ripple {

static
boost::optional<Seed>
validationSeed (Json::Value const& params)
{
    if (!params.isMember (jss::secret))
        return randomSeed ();

    return parseGenericSeed (params[jss::secret].asString ());
}

Json::Value doValidationCreate (RPC::Context& context)
{
    Json::Value     obj (Json::objectValue);

    auto seed = validationSeed(context.params);

    if (!seed)
        return rpcError (rpcBAD_SEED);

    auto const private_key = generateSecretKey (KeyType::secp256k1, *seed);

    obj[jss::validation_public_key] = toBase58 (
        TokenType::NodePublic,
        derivePublicKey (KeyType::secp256k1, private_key));

    obj[jss::validation_private_key] = toBase58 (
        TokenType::NodePrivate, private_key);

    obj[jss::validation_seed] = toBase58 (*seed);
    obj[jss::validation_key] = seedAs1751 (*seed);

    return obj;
}

} 
























