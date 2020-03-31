

#include <ripple/app/main/Application.h>
#include <ripple/basics/StringUtilities.h>
#include <ripple/ledger/ReadView.h>
#include <ripple/net/RPCErr.h>
#include <ripple/protocol/ErrorCodes.h>
#include <ripple/protocol/jss.h>
#include <ripple/protocol/PayChan.h>
#include <ripple/protocol/STAccount.h>
#include <ripple/resource/Fees.h>
#include <ripple/rpc/Context.h>
#include <ripple/rpc/impl/RPCHelpers.h>
#include <ripple/rpc/impl/Tuning.h>

#include <boost/optional.hpp>

namespace ripple {

Json::Value doChannelAuthorize (RPC::Context& context)
{
    auto const& params (context.params);
    for (auto const& p : {jss::secret, jss::channel_id, jss::amount})
        if (!params.isMember (p))
            return RPC::missing_field_error (p);

    Json::Value result;
    auto const keypair = RPC::keypairForSignature (params, result);
    if (RPC::contains_error (result))
        return result;

    uint256 channelId;
    if (!channelId.SetHexExact (params[jss::channel_id].asString ()))
        return rpcError (rpcCHANNEL_MALFORMED);

    boost::optional<std::uint64_t> const optDrops =
        params[jss::amount].isString()
        ? to_uint64(params[jss::amount].asString())
        : boost::none;

    if (!optDrops)
        return rpcError (rpcCHANNEL_AMT_MALFORMED);

    std::uint64_t const drops = *optDrops;

    Serializer msg;
    serializePayChanAuthorization (msg, channelId, XRPAmount (drops));

    try
    {
        auto const buf = sign (keypair.first, keypair.second, msg.slice ());
        result[jss::signature] = strHex (buf);
    }
    catch (std::exception&)
    {
        result =
            RPC::make_error (rpcINTERNAL, "Exception occurred during signing.");
    }
    return result;
}

Json::Value doChannelVerify (RPC::Context& context)
{
    auto const& params (context.params);
    for (auto const& p :
         {jss::public_key, jss::channel_id, jss::amount, jss::signature})
        if (!params.isMember (p))
            return RPC::missing_field_error (p);

    boost::optional<PublicKey> pk;
    {
        std::string const strPk = params[jss::public_key].asString();
        pk = parseBase58<PublicKey>(TokenType::AccountPublic, strPk);

        if (!pk)
        {
            std::pair<Blob, bool> pkHex(strUnHex (strPk));
            if (!pkHex.second)
                return rpcError(rpcPUBLIC_MALFORMED);
            auto const pkType = publicKeyType(makeSlice(pkHex.first));
            if (!pkType)
                return rpcError(rpcPUBLIC_MALFORMED);
            pk.emplace(makeSlice(pkHex.first));
        }
    }

    uint256 channelId;
    if (!channelId.SetHexExact (params[jss::channel_id].asString ()))
        return rpcError (rpcCHANNEL_MALFORMED);

    boost::optional<std::uint64_t> const optDrops =
        params[jss::amount].isString()
        ? to_uint64(params[jss::amount].asString())
        : boost::none;

    if (!optDrops)
        return rpcError (rpcCHANNEL_AMT_MALFORMED);

    std::uint64_t const drops = *optDrops;

    std::pair<Blob, bool> sig(strUnHex (params[jss::signature].asString ()));
    if (!sig.second || !sig.first.size ())
        return rpcError (rpcINVALID_PARAMS);

    Serializer msg;
    serializePayChanAuthorization (msg, channelId, XRPAmount (drops));

    Json::Value result;
    result[jss::signature_verified] =
        verify (*pk, msg.slice (), makeSlice (sig.first),  true);
    return result;
}

} 
























