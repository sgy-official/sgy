

#include <ripple/app/main/Application.h>
#include <ripple/app/paths/RippleState.h>
#include <ripple/ledger/ReadView.h>
#include <ripple/net/RPCErr.h>
#include <ripple/protocol/ErrorCodes.h>
#include <ripple/protocol/jss.h>
#include <ripple/rpc/Context.h>
#include <ripple/rpc/impl/RPCHelpers.h>

namespace ripple {

Json::Value doAccountCurrencies (RPC::Context& context)
{
    auto& params = context.params;

    std::shared_ptr<ReadView const> ledger;
    auto result = RPC::lookupLedger (ledger, context);
    if (!ledger)
        return result;

    if (!(params.isMember (jss::account) || params.isMember (jss::ident)))
        return RPC::missing_field_error (jss::account);

    std::string const strIdent (params.isMember (jss::account)
        ? params[jss::account].asString ()
        : params[jss::ident].asString ());

    bool const bStrict = params.isMember (jss::strict) &&
            params[jss::strict].asBool ();

    AccountID accountID; 
    if (auto jvAccepted = RPC::accountFromString (accountID, strIdent, bStrict))
        return jvAccepted;

    if (! ledger->read(keylet::account(accountID)))
        return rpcError (rpcACT_NOT_FOUND);

    std::set<Currency> send, receive;
    for (auto const& item : getRippleStateItems (accountID, *ledger))
    {
        auto const rspEntry = item.get();

        STAmount const& saBalance = rspEntry->getBalance ();

        if (saBalance < rspEntry->getLimit ())
            receive.insert (saBalance.getCurrency ());
        if ((-saBalance) < rspEntry->getLimitPeer ())
            send.insert (saBalance.getCurrency ());
    }

    send.erase (badCurrency());
    receive.erase (badCurrency());

    Json::Value& sendCurrencies =
            (result[jss::send_currencies] = Json::arrayValue);
    for (auto const& c: send)
        sendCurrencies.append (to_string (c));

    Json::Value& recvCurrencies =
            (result[jss::receive_currencies] = Json::arrayValue);
    for (auto const& c: receive)
        recvCurrencies.append (to_string (c));

    return result;
}

} 
























