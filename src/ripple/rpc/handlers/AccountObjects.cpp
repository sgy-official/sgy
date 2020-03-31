

#include <ripple/json/json_writer.h>
#include <ripple/app/main/Application.h>
#include <ripple/ledger/ReadView.h>
#include <ripple/net/RPCErr.h>
#include <ripple/protocol/ErrorCodes.h>
#include <ripple/protocol/Indexes.h>
#include <ripple/protocol/jss.h>
#include <ripple/protocol/LedgerFormats.h>
#include <ripple/resource/Fees.h>
#include <ripple/rpc/Context.h>
#include <ripple/rpc/impl/RPCHelpers.h>
#include <ripple/rpc/impl/Tuning.h>

#include <string>
#include <sstream>

namespace ripple {



Json::Value doAccountObjects (RPC::Context& context)
{
    auto const& params = context.params;
    if (! params.isMember (jss::account))
        return RPC::missing_field_error (jss::account);

    std::shared_ptr<ReadView const> ledger;
    auto result = RPC::lookupLedger (ledger, context);
    if (ledger == nullptr)
        return result;

    AccountID accountID;
    {
        auto const strIdent = params[jss::account].asString ();
        if (auto jv = RPC::accountFromString (accountID, strIdent))
        {
            for (auto it = jv.begin (); it != jv.end (); ++it)
                result[it.memberName ()] = *it;

            return result;
        }
    }

    if (! ledger->exists(keylet::account (accountID)))
        return rpcError (rpcACT_NOT_FOUND);

    auto type = RPC::chooseLedgerEntryType(params);
    if (type.first)
    {
        result.clear();
        type.first.inject(result);
        return result;
    }

    unsigned int limit;
    if (auto err = readLimitField(limit, RPC::Tuning::accountObjects, context))
        return *err;

    uint256 dirIndex;
    uint256 entryIndex;
    if (params.isMember (jss::marker))
    {
        auto const& marker = params[jss::marker];
        if (! marker.isString ())
            return RPC::expected_field_error (jss::marker, "string");

        std::stringstream ss (marker.asString ());
        std::string s;
        if (!std::getline(ss, s, ','))
            return RPC::invalid_field_error (jss::marker);

        if (! dirIndex.SetHex (s))
            return RPC::invalid_field_error (jss::marker);

        if (! std::getline (ss, s, ','))
            return RPC::invalid_field_error (jss::marker);

        if (! entryIndex.SetHex (s))
            return RPC::invalid_field_error (jss::marker);
    }

    if (! RPC::getAccountObjects (*ledger, accountID, type.second,
        dirIndex, entryIndex, limit, result))
    {
        result[jss::account_objects] = Json::arrayValue;
    }

    result[jss::account] = context.app.accountIDCache().toBase58 (accountID);
    context.loadType = Resource::feeMediumBurdenRPC;
    return result;
}

} 
























