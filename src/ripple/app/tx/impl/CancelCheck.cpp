

#include <ripple/app/tx/impl/CancelCheck.h>

#include <ripple/app/ledger/Ledger.h>
#include <ripple/basics/Log.h>
#include <ripple/ledger/ApplyView.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/Indexes.h>
#include <ripple/protocol/STAccount.h>
#include <ripple/protocol/TER.h>
#include <ripple/protocol/TxFlags.h>

namespace ripple {

NotTEC
CancelCheck::preflight (PreflightContext const& ctx)
{
    if (! ctx.rules.enabled (featureChecks))
        return temDISABLED;

    NotTEC const ret {preflight1 (ctx)};
    if (! isTesSuccess (ret))
        return ret;

    if (ctx.tx.getFlags() & tfUniversalMask)
    {
        JLOG(ctx.j.warn()) << "Malformed transaction: Invalid flags set.";
        return temINVALID_FLAG;
    }

    return preflight2 (ctx);
}

TER
CancelCheck::preclaim (PreclaimContext const& ctx)
{
    auto const sleCheck = ctx.view.read (keylet::check (ctx.tx[sfCheckID]));
    if (! sleCheck)
    {
        JLOG(ctx.j.warn()) << "Check does not exist.";
        return tecNO_ENTRY;
    }

    using duration = NetClock::duration;
    using timepoint = NetClock::time_point;
    auto const optExpiry = (*sleCheck)[~sfExpiration];

    if (! optExpiry ||
        (ctx.view.parentCloseTime() < timepoint {duration {*optExpiry}}))
    {
        AccountID const acctId {ctx.tx[sfAccount]};
        if (acctId != (*sleCheck)[sfAccount] &&
            acctId != (*sleCheck)[sfDestination])
        {
            JLOG(ctx.j.warn()) << "Check is not expired and canceler is "
                "neither check source nor destination.";
            return tecNO_PERMISSION;
        }
    }
    return tesSUCCESS;
}

TER
CancelCheck::doApply ()
{
    uint256 const checkId {ctx_.tx[sfCheckID]};
    auto const sleCheck = view().peek (keylet::check (checkId));
    if (! sleCheck)
    {
        JLOG(j_.warn()) << "Check does not exist.";
        return tecNO_ENTRY;
    }

    AccountID const srcId {sleCheck->getAccountID (sfAccount)};
    AccountID const dstId {sleCheck->getAccountID (sfDestination)};
    auto viewJ = ctx_.app.journal ("View");

    if (srcId != dstId)
    {
        std::uint64_t const page {(*sleCheck)[sfDestinationNode]};
        if (! view().dirRemove (keylet::ownerDir(dstId), page, checkId, true))
        {
            JLOG(j_.warn()) << "Unable to delete check from destination.";
            return tefBAD_LEDGER;
        }
    }
    {
        std::uint64_t const page {(*sleCheck)[sfOwnerNode]};
        if (! view().dirRemove (keylet::ownerDir(srcId), page, checkId, true))
        {
            JLOG(j_.warn()) << "Unable to delete check from owner.";
            return tefBAD_LEDGER;
        }
    }

    auto const sleSrc = view().peek (keylet::account (srcId));
    adjustOwnerCount (view(), sleSrc, -1, viewJ);

    view().erase (sleCheck);
    return tesSUCCESS;
}

} 
























