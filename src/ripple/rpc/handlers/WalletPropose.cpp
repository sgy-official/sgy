

#include <ripple/basics/strHex.h>
#include <ripple/crypto/KeyType.h>
#include <ripple/net/RPCErr.h>
#include <ripple/protocol/ErrorCodes.h>
#include <ripple/protocol/jss.h>
#include <ripple/protocol/PublicKey.h>
#include <ripple/protocol/SecretKey.h>
#include <ripple/protocol/Seed.h>
#include <ripple/rpc/Context.h>
#include <ripple/rpc/impl/RPCHelpers.h>
#include <ripple/rpc/handlers/WalletPropose.h>
#include <ed25519-donna/ed25519.h>
#include <boost/optional.hpp>
#include <cmath>
#include <map>

namespace ripple {

double
estimate_entropy (std::string const& input)
{
    std::map<int, double> freq;

    for (auto const& c : input)
        freq[c]++;

    double se = 0.0;

    for (auto const& f : freq)
    {
        auto x = f.second / input.length();
        se += (x) * log2(x);
    }

    return std::floor (-se * input.length());
}

Json::Value doWalletPropose (RPC::Context& context)
{
    return walletPropose (context.params);
}

Json::Value walletPropose (Json::Value const& params)
{
    boost::optional<KeyType> keyType;
    boost::optional<Seed> seed;
    bool rippleLibSeed = false;

    if (params.isMember (jss::key_type))
    {
        if (! params[jss::key_type].isString())
        {
            return RPC::expected_field_error (
                jss::key_type, "string");
        }

        keyType = keyTypeFromString (
            params[jss::key_type].asString());

        if (!keyType)
            return rpcError(rpcINVALID_PARAMS);
    }

    {
        if (params.isMember(jss::passphrase))
            seed = RPC::parseRippleLibSeed(params[jss::passphrase]);
        else if (params.isMember(jss::seed))
            seed = RPC::parseRippleLibSeed(params[jss::seed]);

        if(seed)
        {
            rippleLibSeed = true;

            if (keyType.value_or(KeyType::ed25519) != KeyType::ed25519)
                return rpcError(rpcBAD_SEED);

            keyType = KeyType::ed25519;
        }
    }

    if (!seed)
    {
        if (params.isMember(jss::passphrase) ||
            params.isMember(jss::seed) ||
            params.isMember(jss::seed_hex))
        {
            Json::Value err;

            seed = RPC::getSeedFromRPC(params, err);

            if (!seed)
                return err;
        }
        else
        {
            seed = randomSeed();
        }
    }

    if (!keyType)
        keyType = KeyType::secp256k1;

    auto const publicKey = generateKeyPair (*keyType, *seed).first;

    Json::Value obj (Json::objectValue);

    auto const seed1751 = seedAs1751 (*seed);
    auto const seedHex = strHex (*seed);
    auto const seedBase58 = toBase58 (*seed);

    obj[jss::master_seed] = seedBase58;
    obj[jss::master_seed_hex] = seedHex;
    obj[jss::master_key] = seed1751;
    obj[jss::account_id] = toBase58(calcAccountID(publicKey));
    obj[jss::public_key] = toBase58(TokenType::AccountPublic, publicKey);
    obj[jss::key_type] = to_string (*keyType);
    obj[jss::public_key_hex] = strHex (publicKey);

    if (!rippleLibSeed && params.isMember (jss::passphrase))
    {
        auto const passphrase = params[jss::passphrase].asString();

        if (passphrase != seed1751 &&
            passphrase != seedBase58 &&
            passphrase != seedHex)
        {
            if (estimate_entropy (passphrase) < 80.0)
                obj[jss::warning] =
                    "This wallet was generated using a user-supplied "
                    "passphrase that has low entropy and is vulnerable "
                    "to brute-force attacks.";
            else
                obj[jss::warning] =
                    "This wallet was generated using a user-supplied "
                    "passphrase. It may be vulnerable to brute-force "
                    "attacks.";
        }
    }

    return obj;
}

} 
























