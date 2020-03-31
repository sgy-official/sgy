

#include <ripple/app/tx/impl/CancelTicket.h>
#include <ripple/basics/Log.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/Indexes.h>
#include <ripple/protocol/TxFlags.h>
#include <ripple/ledger/View.h>

namespace ripple {

NotTEC
CancelTicket::preflight (PreflightContext const& ctx)
{
    if (! ctx.rules.enabled(featureTickets))
        return temDISABLED;

    if (ctx.tx.getFlags() & tfUniversalMask)
        return temINVALID_FLAG;

    auto const ret = preflight1 (ctx);
    if (!isTesSuccess (ret))
        return ret;

    return preflight2 (ctx);
}

TER
CancelTicket::doApply ()
{
    uint256 const ticketId = ctx_.tx.getFieldH256 (sfTicketID);

    SLE::pointer sleTicket = view().peek (keylet::ticket(ticketId));

    if (!sleTicket)
        return tecNO_ENTRY;

    auto const ticket_owner =
        sleTicket->getAccountID (sfAccount);

    bool authorized =
        account_ == ticket_owner;

    if (!authorized && sleTicket->isFieldPresent (sfTarget))
        authorized = (account_ == sleTicket->getAccountID (sfTarget));

    if (!authorized && sleTicket->isFieldPresent (sfExpiration))
    {
        using tp = NetClock::time_point;
        using d = tp::duration;
        auto const expiration = tp{d{sleTicket->getFieldU32(sfExpiration)}};

        if (view().parentCloseTime() >= expiration)
            authorized = true;
    }

    if (!authorized)
        return tecNO_PERMISSION;

    std::uint64_t const hint (sleTicket->getFieldU64 (sfOwnerNode));

    if (! ctx_.view().dirRemove(
            keylet::ownerDir(ticket_owner), hint, ticketId, false))
    {
        return tefBAD_LEDGER;
    }

    auto viewJ = ctx_.app.journal ("View");
    adjustOwnerCount(view(), view().peek(
        keylet::account(ticket_owner)), -1, viewJ);
    ctx_.view ().erase (sleTicket);

    return tesSUCCESS;
}

}
























