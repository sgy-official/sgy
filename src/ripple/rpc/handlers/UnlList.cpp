

#include <ripple/app/main/Application.h>
#include <ripple/app/misc/ValidatorList.h>
#include <ripple/protocol/jss.h>
#include <ripple/rpc/Context.h>
#include <ripple/basics/make_lock.h>

namespace ripple {

Json::Value doUnlList (RPC::Context& context)
{
    Json::Value obj (Json::objectValue);

    context.app.validators().for_each_listed (
        [&unl = obj[jss::unl]](
            PublicKey const& publicKey,
            bool trusted)
        {
            Json::Value node (Json::objectValue);

            node[jss::pubkey_validator] = toBase58(
                TokenType::NodePublic, publicKey);
            node[jss::trusted] = trusted;

            unl.append (std::move (node));
        });

    return obj;
}

} 
























