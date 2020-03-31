

#include <ripple/app/tx/impl/CashCheck.h>
#include <ripple/app/ledger/Ledger.h>
#include <ripple/app/paths/Flow.h>
#include <ripple/basics/Log.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/Indexes.h>
#include <ripple/protocol/STAccount.h>
#include <ripple/protocol/TER.h>
#include <ripple/protocol/TxFlags.h>

#include <algorithm>

namespace ripple {

NotTEC
CashCheck::preflight (PreflightContext const& ctx)
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

    auto const optAmount = ctx.tx[~sfAmount];
    auto const optDeliverMin = ctx.tx[~sfDeliverMin];

    if (static_cast<bool>(optAmount) == static_cast<bool>(optDeliverMin))
    {
        JLOG(ctx.j.warn()) << "Malformed transaction: "
            "does not specify exactly one of Amount and DeliverMin.";
        return temMALFORMED;
    }

    STAmount const value {optAmount ? *optAmount : *optDeliverMin};
    if (!isLegalNet (value) || value.signum() <= 0)
    {
        JLOG(ctx.j.warn()) << "Malformed transaction: bad amount: "
            << value.getFullText();
        return temBAD_AMOUNT;
    }

    if (badCurrency() == value.getCurrency())
    {
        JLOG(ctx.j.warn()) <<"Malformed transaction: Bad currency.";
        return temBAD_CURRENCY;
    }

    return preflight2 (ctx);
}

TER
CashCheck::preclaim (PreclaimContext const& ctx)
{
    auto const sleCheck = ctx.view.read (keylet::check (ctx.tx[sfCheckID]));
    if (! sleCheck)
    {
        JLOG(ctx.j.warn()) << "Check does not exist.";
        return tecNO_ENTRY;
    }

    AccountID const dstId {(*sleCheck)[sfDestination]};
    if (ctx.tx[sfAccount] != dstId)
    {
        JLOG(ctx.j.warn()) << "Cashing a check with wrong Destination.";
        return tecNO_PERMISSION;
    }
    AccountID const srcId {(*sleCheck)[sfAccount]};
    if (srcId == dstId)
    {
        JLOG(ctx.j.error()) << "Malformed transaction: Cashing check to self.";
        return tecINTERNAL;
    }
    {
        auto const sleSrc = ctx.view.read (keylet::account (srcId));
        auto const sleDst = ctx.view.read (keylet::account (dstId));
        if (!sleSrc || !sleDst)
        {
            JLOG(ctx.j.warn())
                << "Malformed transaction: source or destination not in ledger";
            return tecNO_ENTRY;
        }

        if ((sleDst->getFlags() & lsfRequireDestTag) &&
            !sleCheck->isFieldPresent (sfDestinationTag))
        {
            JLOG(ctx.j.warn())
                << "Malformed transaction: DestinationTag required in check.";
            return tecDST_TAG_NEEDED;
        }
    }
    {
        using duration = NetClock::duration;
        using timepoint = NetClock::time_point;
        auto const optExpiry = (*sleCheck)[~sfExpiration];

        if (optExpiry &&
            (ctx.view.parentCloseTime() >= timepoint {duration {*optExpiry}}))
        {
            JLOG(ctx.j.warn()) << "Cashing a check that has already expired.";
            return tecEXPIRED;
        }
    }
    {
        STAmount const value {[] (STTx const& tx)
            {
                auto const optAmount = tx[~sfAmount];
                return optAmount ? *optAmount : tx[sfDeliverMin];
            } (ctx.tx)};

        STAmount const sendMax {(*sleCheck)[sfSendMax]};
        Currency const currency {value.getCurrency()};
        if (currency != sendMax.getCurrency())
        {
            JLOG(ctx.j.warn()) << "Check cash does not match check currency.";
            return temMALFORMED;
        }
        AccountID const issuerId {value.getIssuer()};
        if (issuerId != sendMax.getIssuer())
        {
            JLOG(ctx.j.warn()) << "Check cash does not match check issuer.";
            return temMALFORMED;
        }
        if (value > sendMax)
        {
            JLOG(ctx.j.warn()) << "Check cashed for more than check sendMax.";
            return tecPATH_PARTIAL;
        }

        {
            STAmount availableFunds {accountFunds (ctx.view,
                (*sleCheck)[sfAccount], value, fhZERO_IF_FROZEN, ctx.j)};

            if (value.native())
                availableFunds += XRPAmount (ctx.view.fees().increment);

            if (value > availableFunds)
            {
                JLOG(ctx.j.warn())
                    << "Check cashed for more than owner's balance.";
                return tecPATH_PARTIAL;
            }
        }

        if (! value.native() && (value.getIssuer() != dstId))
        {
            auto const sleTrustLine = ctx.view.read (
                keylet::line (dstId, issuerId, currency));
            if (! sleTrustLine)
            {
                JLOG(ctx.j.warn())
                    << "Cannot cash check for IOU without trustline.";
                return tecNO_LINE;
            }

            auto const sleIssuer = ctx.view.read (keylet::account (issuerId));
            if (! sleIssuer)
            {
                JLOG(ctx.j.warn())
                    << "Can't receive IOUs from non-existent issuer: "
                    << to_string (issuerId);
                return tecNO_ISSUER;
            }

            if ((*sleIssuer)[sfFlags] & lsfRequireAuth)
            {
                bool const canonical_gt (dstId > issuerId);

                bool const is_authorized ((*sleTrustLine)[sfFlags] &
                    (canonical_gt ? lsfLowAuth : lsfHighAuth));

                if (! is_authorized)
                {
                    JLOG(ctx.j.warn())
                        << "Can't receive IOUs from issuer without auth.";
                    return tecNO_AUTH;
                }
            }


            if (isFrozen (ctx.view, dstId, currency, issuerId))
            {
                JLOG(ctx.j.warn())
                    << "Cashing a check to a frozen trustline.";
                return tecFROZEN;
            }
        }
    }
    return tesSUCCESS;
}

TER
CashCheck::doApply ()
{
    PaymentSandbox psb (&ctx_.view());

    uint256 const checkKey {ctx_.tx[sfCheckID]};
    auto const sleCheck = psb.peek (keylet::check (checkKey));
    if (! sleCheck)
    {
        JLOG(j_.fatal())
            << "Precheck did not verify check's existence.";
        return tecFAILED_PROCESSING;
    }

    AccountID const srcId {sleCheck->getAccountID (sfAccount)};
    auto const sleSrc = psb.peek (keylet::account (srcId));
    auto const sleDst = psb.peek (keylet::account (account_));

    if (!sleSrc || !sleDst)
    {
        JLOG(ctx_.journal.fatal())
            << "Precheck did not verify source or destination's existence.";
        return tecFAILED_PROCESSING;
    }

    auto viewJ = ctx_.app.journal ("View");
    auto const optDeliverMin = ctx_.tx[~sfDeliverMin];
    bool const doFix1623 {ctx_.view().rules().enabled (fix1623)};
    if (srcId != account_)
    {
        STAmount const sendMax {sleCheck->getFieldAmount (sfSendMax)};

        if (sendMax.native())
        {
            STAmount const srcLiquid {xrpLiquid (psb, srcId, -1, viewJ)};

            STAmount const xrpDeliver {optDeliverMin ?
                std::max (*optDeliverMin, std::min (sendMax, srcLiquid)) :
                ctx_.tx.getFieldAmount (sfAmount)};

            if (srcLiquid < xrpDeliver)
            {
                JLOG(j_.trace()) << "Cash Check: Insufficient XRP: "
                    << srcLiquid.getFullText()
                    << " < " << xrpDeliver.getFullText();
                return tecUNFUNDED_PAYMENT;
            }

            if (optDeliverMin && doFix1623)
                ctx_.deliver (xrpDeliver);

            transferXRP (psb, srcId, account_, xrpDeliver, viewJ);
        }
        else
        {
            STAmount const flowDeliver {optDeliverMin ?
                STAmount { optDeliverMin->issue(),
                    STAmount::cMaxValue / 2, STAmount::cMaxOffset } :
                static_cast<STAmount>(ctx_.tx[sfAmount])};

            auto const result = flow (psb, flowDeliver, srcId, account_,
                STPathSet{},
                true,                              
                static_cast<bool>(optDeliverMin),  
                true,                              
                false,                             
                boost::none,
                sleCheck->getFieldAmount (sfSendMax),
                viewJ);

            if (result.result() != tesSUCCESS)
            {
                JLOG(ctx_.journal.warn())
                    << "flow failed when cashing check.";
                return result.result();
            }

            if (optDeliverMin)
            {
                if (result.actualAmountOut < *optDeliverMin)
                {
                    JLOG(ctx_.journal.warn()) << "flow did not produce DeliverMin.";
                    return tecPATH_PARTIAL;
                }
                if (doFix1623)
                    ctx_.deliver (result.actualAmountOut);
            }
        }
    }

    if (srcId != account_)
    {
        std::uint64_t const page {(*sleCheck)[sfDestinationNode]};
        if (! ctx_.view().dirRemove(
                keylet::ownerDir(account_), page, checkKey, true))
        {
            JLOG(j_.warn()) << "Unable to delete check from destination.";
            return tefBAD_LEDGER;
        }
    }
    {
        std::uint64_t const page {(*sleCheck)[sfOwnerNode]};
        if (! ctx_.view().dirRemove(
                keylet::ownerDir(srcId), page, checkKey, true))
        {
            JLOG(j_.warn()) << "Unable to delete check from owner.";
            return tefBAD_LEDGER;
        }
    }
    adjustOwnerCount (psb, sleSrc, -1, viewJ);

    psb.erase (sleCheck);

    psb.apply (ctx_.rawView());
    return tesSUCCESS;
}

} 
























