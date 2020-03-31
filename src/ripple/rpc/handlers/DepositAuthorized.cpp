


#include <ripple/ledger/ReadView.h>
#include <ripple/protocol/ErrorCodes.h>
#include <ripple/protocol/Indexes.h>
#include <ripple/protocol/jss.h>
#include <ripple/rpc/Context.h>
#include <ripple/rpc/impl/RPCHelpers.h>

namespace ripple {


Json::Value doDepositAuthorized (RPC::Context& context)
{
    Json::Value const& params = context.params;

    if (! params.isMember (jss::source_account))
        return RPC::missing_field_error (jss::source_account);
    if (! params[jss::source_account].isString())
        return RPC::make_error (rpcINVALID_PARAMS,
            RPC::expected_field_message (jss::source_account,
                "a string"));

    AccountID srcAcct;
    {
        Json::Value const jvAccepted = RPC::accountFromString (
            srcAcct, params[jss::source_account].asString(), true);
        if (jvAccepted)
            return jvAccepted;
    }

    if (! params.isMember (jss::destination_account))
        return RPC::missing_field_error (jss::destination_account);
    if (! params[jss::destination_account].isString())
        return RPC::make_error (rpcINVALID_PARAMS,
            RPC::expected_field_message (jss::destination_account,
                "a string"));

    AccountID dstAcct;
    {
        Json::Value const jvAccepted = RPC::accountFromString (
            dstAcct, params[jss::destination_account].asString(), true);
        if (jvAccepted)
            return jvAccepted;
    }

    std::shared_ptr<ReadView const> ledger;
    Json::Value result = RPC::lookupLedger (ledger, context);

    if (!ledger)
        return result;

    if (! ledger->exists (keylet::account(srcAcct)))
    {
        RPC::inject_error (rpcSRC_ACT_NOT_FOUND, result);
        return result;
    }

    auto const sleDest = ledger->read (keylet::account(dstAcct));
    if (! sleDest)
    {
        RPC::inject_error (rpcDST_ACT_NOT_FOUND, result);
        return result;
    }

    bool depositAuthorized {true};
    if (srcAcct != dstAcct)
    {
        if (sleDest->getFlags() & lsfDepositAuth)
        {
            auto const sleDepositAuth =
                ledger->read(keylet::depositPreauth (dstAcct, srcAcct));
            depositAuthorized = static_cast<bool>(sleDepositAuth);
        }
    }
    result[jss::source_account] = params[jss::source_account].asString();
    result[jss::destination_account] =
        params[jss::destination_account].asString();

    result[jss::deposit_authorized] = depositAuthorized;
    return result;
}

} 
























