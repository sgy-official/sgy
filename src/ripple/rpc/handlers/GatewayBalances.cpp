

#include <ripple/app/main/Application.h>
#include <ripple/app/paths/RippleState.h>
#include <ripple/ledger/ReadView.h>
#include <ripple/protocol/AccountID.h>
#include <ripple/protocol/ErrorCodes.h>
#include <ripple/protocol/jss.h>
#include <ripple/protocol/PublicKey.h>
#include <ripple/resource/Fees.h>
#include <ripple/rpc/Context.h>
#include <ripple/rpc/impl/RPCHelpers.h>

namespace ripple {




Json::Value doGatewayBalances (RPC::Context& context)
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
    auto jvAccepted = RPC::accountFromString (accountID, strIdent, bStrict);

    if (jvAccepted)
        return jvAccepted;

    context.loadType = Resource::feeHighBurdenRPC;

    result[jss::account] = context.app.accountIDCache().toBase58 (accountID);

    std::set <AccountID> hotWallets;

    if (params.isMember (jss::hotwallet))
    {

        auto addHotWallet = [&hotWallets](Json::Value const& j)
        {
            if (j.isString())
            {
                auto const pk = parseBase58<PublicKey>(
                    TokenType::AccountPublic,
                    j.asString ());
                if (pk)
                {
                    hotWallets.insert(calcAccountID(*pk));
                    return true;
                }

                auto const id = parseBase58<AccountID>(j.asString());

                if (id)
                {
                    hotWallets.insert(*id);
                    return true;
                }
            }

            return false;
        };

        Json::Value const& hw = params[jss::hotwallet];
        bool valid = true;

        if (hw.isArrayOrNull())
        {
            for (unsigned i = 0; i < hw.size(); ++i)
                valid &= addHotWallet (hw[i]);
        }
        else if (hw.isString())
        {
            valid &= addHotWallet (hw);
        }
        else
        {
            valid = false;
        }

        if (! valid)
        {
            result[jss::error]   = "invalidHotWallet";
            return result;
        }

    }

    std::map <Currency, STAmount> sums;
    std::map <AccountID, std::vector <STAmount>> hotBalances;
    std::map <AccountID, std::vector <STAmount>> assets;
    std::map <AccountID, std::vector <STAmount>> frozenBalances;

    {
        forEachItem(*ledger, accountID,
            [&](std::shared_ptr<SLE const> const& sle)
            {
                auto rs = RippleState::makeItem (accountID, sle);

                if (!rs)
                    return;

                int balSign = rs->getBalance().signum();
                if (balSign == 0)
                    return;

                auto const& peer = rs->getAccountIDPeer();


                if (hotWallets.count (peer) > 0)
                {
                    hotBalances[peer].push_back (-rs->getBalance ());
                }
                else if (balSign > 0)
                {
                    assets[peer].push_back (rs->getBalance ());
                }
                else if (rs->getFreeze())
                {
                    frozenBalances[peer].push_back (-rs->getBalance ());
                }
                else
                {
                    auto& bal = sums[rs->getBalance().getCurrency()];
                    if (bal == beast::zero)
                    {
                        bal = -rs->getBalance();
                    }
                    else
                        bal -= rs->getBalance();
                }
            });
    }

    if (! sums.empty())
    {
        Json::Value j;
        for (auto const& e : sums)
        {
            j[to_string (e.first)] = e.second.getText ();
        }
        result [jss::obligations] = std::move (j);
    }

    auto populate = [](
        std::map <AccountID, std::vector <STAmount>> const& array,
        Json::Value& result,
        Json::StaticString const& name)
        {
            if (!array.empty())
            {
                Json::Value j;
                for (auto const& account : array)
                {
                    Json::Value balanceArray;
                    for (auto const& balance : account.second)
                    {
                        Json::Value entry;
                        entry[jss::currency] = to_string (balance.issue ().currency);
                        entry[jss::value] = balance.getText();
                        balanceArray.append (std::move (entry));
                    }
                    j [to_string (account.first)] = std::move (balanceArray);
                }
                result [name] = std::move (j);
            }
        };

    populate (hotBalances, result, jss::balances);
    populate (frozenBalances, result, jss::frozen_balances);
    populate (assets, result, jss::assets);

    return result;
}

} 
























