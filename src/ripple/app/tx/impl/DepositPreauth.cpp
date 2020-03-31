

#include <ripple/app/tx/impl/DepositPreauth.h>
#include <ripple/basics/Log.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/Indexes.h>
#include <ripple/protocol/st.h>
#include <ripple/protocol/TxFlags.h>
#include <ripple/ledger/View.h>

namespace ripple {

NotTEC
DepositPreauth::preflight (PreflightContext const& ctx)
{
    if (! ctx.rules.enabled (featureDepositPreauth))
        return temDISABLED;

    auto const ret = preflight1 (ctx);
    if (!isTesSuccess (ret))
        return ret;

    auto& tx = ctx.tx;
    auto& j = ctx.j;

    if (tx.getFlags() & tfUniversalMask)
    {
        JLOG(j.trace()) <<
            "Malformed transaction: Invalid flags set.";
        return temINVALID_FLAG;
    }

    auto const optAuth = ctx.tx[~sfAuthorize];
    auto const optUnauth = ctx.tx[~sfUnauthorize];
    if (static_cast<bool>(optAuth) == static_cast<bool>(optUnauth))
    {
        JLOG(j.trace()) <<
            "Malformed transaction: "
            "Invalid Authorize and Unauthorize field combination.";
        return temMALFORMED;
    }

    AccountID const target {optAuth ? *optAuth : *optUnauth};
    if (target == beast::zero)
    {
        JLOG(j.trace()) <<
            "Malformed transaction: Authorized or Unauthorized field zeroed.";
        return temINVALID_ACCOUNT_ID;
    }

    if (optAuth && (target == ctx.tx[sfAccount]))
    {
        JLOG(j.trace()) <<
            "Malformed transaction: Attempting to DepositPreauth self.";
        return temCANNOT_PREAUTH_SELF;
    }

    return preflight2 (ctx);
}

TER
DepositPreauth::preclaim(PreclaimContext const& ctx)
{
    if (ctx.tx.isFieldPresent (sfAuthorize))
    {
        AccountID const auth {ctx.tx[sfAuthorize]};
        if (! ctx.view.exists (keylet::account (auth)))
            return tecNO_TARGET;

        if (ctx.view.exists (
            keylet::depositPreauth (ctx.tx[sfAccount], auth)))
            return tecDUPLICATE;
    }
    else
    {
        AccountID const unauth {ctx.tx[sfUnauthorize]};
        if (! ctx.view.exists (
            keylet::depositPreauth (ctx.tx[sfAccount], unauth)))
            return tecNO_ENTRY;
    }
    return tesSUCCESS;
}

TER
DepositPreauth::doApply ()
{
    auto const sleOwner = view().peek (keylet::account (account_));

    if (ctx_.tx.isFieldPresent (sfAuthorize))
    {
        {
            STAmount const reserve {view().fees().accountReserve (
                sleOwner->getFieldU32 (sfOwnerCount) + 1)};

            if (mPriorBalance < reserve)
                return tecINSUFFICIENT_RESERVE;
        }

        AccountID const auth {ctx_.tx[sfAuthorize]};
        auto slePreauth =
            std::make_shared<SLE>(keylet::depositPreauth (account_, auth));

        slePreauth->setAccountID (sfAccount, account_);
        slePreauth->setAccountID (sfAuthorize, auth);
        view().insert (slePreauth);

        auto viewJ = ctx_.app.journal ("View");
        auto const page = view().dirInsert (keylet::ownerDir (account_),
            slePreauth->key(), describeOwnerDir (account_));

        JLOG(j_.trace())
            << "Adding DepositPreauth to owner directory "
            << to_string (slePreauth->key())
            << ": " << (page ? "success" : "failure");

        if (! page)
            return tecDIR_FULL;

        slePreauth->setFieldU64 (sfOwnerNode, *page);

        adjustOwnerCount (view(), sleOwner, 1, viewJ);
    }
    else
    {
        AccountID const unauth {ctx_.tx[sfUnauthorize]};
        uint256 const preauthIndex {getDepositPreauthIndex (account_, unauth)};
        auto slePreauth = view().peek (keylet::depositPreauth (preauthIndex));

        if (! slePreauth)
        {
            JLOG(j_.warn()) << "Selected DepositPreauth does not exist.";
            return tecNO_ENTRY;
        }

        auto viewJ = ctx_.app.journal ("View");
        std::uint64_t const page {(*slePreauth)[sfOwnerNode]};
        if (! view().dirRemove (
            keylet::ownerDir (account_), page, preauthIndex, true))
        {
            JLOG(j_.warn()) << "Unable to delete DepositPreauth from owner.";
            return tefBAD_LEDGER;
        }

        auto const sleOwner = view().peek (keylet::account (account_));
        adjustOwnerCount (view(), sleOwner, -1, viewJ);

        view().erase (slePreauth);
    }
    return tesSUCCESS;
}

} 
























