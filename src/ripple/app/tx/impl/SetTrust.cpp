

#include <ripple/app/tx/impl/SetTrust.h>
#include <ripple/basics/Log.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/Quality.h>
#include <ripple/protocol/Indexes.h>
#include <ripple/protocol/st.h>
#include <ripple/ledger/View.h>

namespace ripple {

NotTEC
SetTrust::preflight (PreflightContext const& ctx)
{
    auto const ret = preflight1 (ctx);
    if (!isTesSuccess (ret))
        return ret;

    auto& tx = ctx.tx;
    auto& j = ctx.j;

    std::uint32_t const uTxFlags = tx.getFlags ();

    if (uTxFlags & tfTrustSetMask)
    {
        JLOG(j.trace()) <<
            "Malformed transaction: Invalid flags set.";
        return temINVALID_FLAG;
    }

    STAmount const saLimitAmount (tx.getFieldAmount (sfLimitAmount));

    if (!isLegalNet (saLimitAmount))
        return temBAD_AMOUNT;

    if (saLimitAmount.native ())
    {
        JLOG(j.trace()) <<
            "Malformed transaction: specifies native limit " <<
            saLimitAmount.getFullText ();
        return temBAD_LIMIT;
    }

    if (badCurrency() == saLimitAmount.getCurrency ())
    {
        JLOG(j.trace()) <<
            "Malformed transaction: specifies XRP as IOU";
        return temBAD_CURRENCY;
    }

    if (saLimitAmount < beast::zero)
    {
        JLOG(j.trace()) <<
            "Malformed transaction: Negative credit limit.";
        return temBAD_LIMIT;
    }

    auto const& issuer = saLimitAmount.getIssuer ();

    if (!issuer || issuer == noAccount())
    {
        JLOG(j.trace()) <<
            "Malformed transaction: no destination account.";
        return temDST_NEEDED;
    }

    return preflight2 (ctx);
}

TER
SetTrust::preclaim(PreclaimContext const& ctx)
{
    auto const id = ctx.tx[sfAccount];

    auto const sle = ctx.view.read(
        keylet::account(id));

    std::uint32_t const uTxFlags = ctx.tx.getFlags();

    bool const bSetAuth = (uTxFlags & tfSetfAuth);

    if (bSetAuth && !(sle->getFieldU32(sfFlags) & lsfRequireAuth))
    {
        JLOG(ctx.j.trace()) <<
            "Retry: Auth not required.";
        return tefNO_AUTH_REQUIRED;
    }

    auto const saLimitAmount = ctx.tx[sfLimitAmount];

    auto const currency = saLimitAmount.getCurrency();
    auto const uDstAccountID = saLimitAmount.getIssuer();

    if (id == uDstAccountID)
    {
        auto const sleDelete = ctx.view.read(
            keylet::line(id, uDstAccountID, currency));

        if (!sleDelete)
        {
            JLOG(ctx.j.trace()) <<
                "Malformed transaction: Can not extend credit to self.";
            return temDST_IS_SRC;
        }
    }

    return tesSUCCESS;
}

TER
SetTrust::doApply ()
{
    TER terResult = tesSUCCESS;

    STAmount const saLimitAmount (ctx_.tx.getFieldAmount (sfLimitAmount));
    bool const bQualityIn (ctx_.tx.isFieldPresent (sfQualityIn));
    bool const bQualityOut (ctx_.tx.isFieldPresent (sfQualityOut));

    Currency const currency (saLimitAmount.getCurrency ());
    AccountID uDstAccountID (saLimitAmount.getIssuer ());

    bool const bHigh = account_ > uDstAccountID;

    auto const sle = view().peek(
        keylet::account(account_));

    std::uint32_t const uOwnerCount = sle->getFieldU32 (sfOwnerCount);


    XRPAmount const reserveCreate ((uOwnerCount < 2)
        ? XRPAmount (beast::zero)
        : view().fees().accountReserve(uOwnerCount + 1));

    std::uint32_t uQualityIn (bQualityIn ? ctx_.tx.getFieldU32 (sfQualityIn) : 0);
    std::uint32_t uQualityOut (bQualityOut ? ctx_.tx.getFieldU32 (sfQualityOut) : 0);

    if (bQualityOut && QUALITY_ONE == uQualityOut)
        uQualityOut = 0;

    std::uint32_t const uTxFlags = ctx_.tx.getFlags ();

    bool const bSetAuth = (uTxFlags & tfSetfAuth);
    bool const bSetNoRipple = (uTxFlags & tfSetNoRipple);
    bool const bClearNoRipple  = (uTxFlags & tfClearNoRipple);
    bool const bSetFreeze = (uTxFlags & tfSetFreeze);
    bool const bClearFreeze = (uTxFlags & tfClearFreeze);

    auto viewJ = ctx_.app.journal ("View");

    if (account_ == uDstAccountID)
    {
        SLE::pointer sleDelete = view().peek (
            keylet::line(account_, uDstAccountID, currency));

        JLOG(j_.warn()) <<
            "Clearing redundant line.";

        return trustDelete (view(),
            sleDelete, account_, uDstAccountID, viewJ);
    }

    SLE::pointer sleDst =
        view().peek (keylet::account(uDstAccountID));

    if (!sleDst)
    {
        JLOG(j_.trace()) <<
            "Delay transaction: Destination account does not exist.";
        return tecNO_DST;
    }

    STAmount saLimitAllow = saLimitAmount;
    saLimitAllow.setIssuer (account_);

    SLE::pointer sleRippleState = view().peek (
        keylet::line(account_, uDstAccountID, currency));

    if (sleRippleState)
    {
        STAmount        saLowBalance;
        STAmount        saLowLimit;
        STAmount        saHighBalance;
        STAmount        saHighLimit;
        std::uint32_t   uLowQualityIn;
        std::uint32_t   uLowQualityOut;
        std::uint32_t   uHighQualityIn;
        std::uint32_t   uHighQualityOut;
        auto const& uLowAccountID   = !bHigh ? account_ : uDstAccountID;
        auto const& uHighAccountID  =  bHigh ? account_ : uDstAccountID;
        SLE::ref        sleLowAccount   = !bHigh ? sle : sleDst;
        SLE::ref        sleHighAccount  =  bHigh ? sle : sleDst;


        saLowBalance    = sleRippleState->getFieldAmount (sfBalance);
        saHighBalance   = -saLowBalance;


        sleRippleState->setFieldAmount (!bHigh ? sfLowLimit : sfHighLimit, saLimitAllow);

        saLowLimit  = !bHigh ? saLimitAllow : sleRippleState->getFieldAmount (sfLowLimit);
        saHighLimit =  bHigh ? saLimitAllow : sleRippleState->getFieldAmount (sfHighLimit);


        if (!bQualityIn)
        {

            uLowQualityIn   = sleRippleState->getFieldU32 (sfLowQualityIn);
            uHighQualityIn  = sleRippleState->getFieldU32 (sfHighQualityIn);
        }
        else if (uQualityIn)
        {

            sleRippleState->setFieldU32 (!bHigh ? sfLowQualityIn : sfHighQualityIn, uQualityIn);

            uLowQualityIn   = !bHigh ? uQualityIn : sleRippleState->getFieldU32 (sfLowQualityIn);
            uHighQualityIn  =  bHigh ? uQualityIn : sleRippleState->getFieldU32 (sfHighQualityIn);
        }
        else
        {

            sleRippleState->makeFieldAbsent (!bHigh ? sfLowQualityIn : sfHighQualityIn);

            uLowQualityIn   = !bHigh ? 0 : sleRippleState->getFieldU32 (sfLowQualityIn);
            uHighQualityIn  =  bHigh ? 0 : sleRippleState->getFieldU32 (sfHighQualityIn);
        }

        if (QUALITY_ONE == uLowQualityIn)   uLowQualityIn   = 0;

        if (QUALITY_ONE == uHighQualityIn)  uHighQualityIn  = 0;


        if (!bQualityOut)
        {

            uLowQualityOut  = sleRippleState->getFieldU32 (sfLowQualityOut);
            uHighQualityOut = sleRippleState->getFieldU32 (sfHighQualityOut);
        }
        else if (uQualityOut)
        {

            sleRippleState->setFieldU32 (!bHigh ? sfLowQualityOut : sfHighQualityOut, uQualityOut);

            uLowQualityOut  = !bHigh ? uQualityOut : sleRippleState->getFieldU32 (sfLowQualityOut);
            uHighQualityOut =  bHigh ? uQualityOut : sleRippleState->getFieldU32 (sfHighQualityOut);
        }
        else
        {

            sleRippleState->makeFieldAbsent (!bHigh ? sfLowQualityOut : sfHighQualityOut);

            uLowQualityOut  = !bHigh ? 0 : sleRippleState->getFieldU32 (sfLowQualityOut);
            uHighQualityOut =  bHigh ? 0 : sleRippleState->getFieldU32 (sfHighQualityOut);
        }

        std::uint32_t const uFlagsIn (sleRippleState->getFieldU32 (sfFlags));
        std::uint32_t uFlagsOut (uFlagsIn);

        if (bSetNoRipple && !bClearNoRipple)
        {
            if ((bHigh ? saHighBalance : saLowBalance) >= beast::zero)
                uFlagsOut |= (bHigh ? lsfHighNoRipple : lsfLowNoRipple);

            else if (view().rules().enabled(fix1578))
                return tecNO_PERMISSION;
        }
        else if (bClearNoRipple && !bSetNoRipple)
        {
            uFlagsOut &= ~(bHigh ? lsfHighNoRipple : lsfLowNoRipple);
        }

        if (bSetFreeze && !bClearFreeze && !sle->isFlag  (lsfNoFreeze))
        {
            uFlagsOut           |= (bHigh ? lsfHighFreeze : lsfLowFreeze);
        }
        else if (bClearFreeze && !bSetFreeze)
        {
            uFlagsOut           &= ~(bHigh ? lsfHighFreeze : lsfLowFreeze);
        }

        if (QUALITY_ONE == uLowQualityOut)  uLowQualityOut  = 0;

        if (QUALITY_ONE == uHighQualityOut) uHighQualityOut = 0;

        bool const bLowDefRipple        = sleLowAccount->getFlags() & lsfDefaultRipple;
        bool const bHighDefRipple       = sleHighAccount->getFlags() & lsfDefaultRipple;

        bool const  bLowReserveSet      = uLowQualityIn || uLowQualityOut ||
                                            ((uFlagsOut & lsfLowNoRipple) == 0) != bLowDefRipple ||
                                            (uFlagsOut & lsfLowFreeze) ||
                                            saLowLimit || saLowBalance > beast::zero;
        bool const  bLowReserveClear    = !bLowReserveSet;

        bool const  bHighReserveSet     = uHighQualityIn || uHighQualityOut ||
                                            ((uFlagsOut & lsfHighNoRipple) == 0) != bHighDefRipple ||
                                            (uFlagsOut & lsfHighFreeze) ||
                                            saHighLimit || saHighBalance > beast::zero;
        bool const  bHighReserveClear   = !bHighReserveSet;

        bool const  bDefault            = bLowReserveClear && bHighReserveClear;

        bool const  bLowReserved = (uFlagsIn & lsfLowReserve);
        bool const  bHighReserved = (uFlagsIn & lsfHighReserve);

        bool        bReserveIncrease    = false;

        if (bSetAuth)
        {
            uFlagsOut |= (bHigh ? lsfHighAuth : lsfLowAuth);
        }

        if (bLowReserveSet && !bLowReserved)
        {
            adjustOwnerCount(view(),
                sleLowAccount, 1, viewJ);
            uFlagsOut |= lsfLowReserve;

            if (!bHigh)
                bReserveIncrease = true;
        }

        if (bLowReserveClear && bLowReserved)
        {
            adjustOwnerCount(view(),
                sleLowAccount, -1, viewJ);
            uFlagsOut &= ~lsfLowReserve;
        }

        if (bHighReserveSet && !bHighReserved)
        {
            adjustOwnerCount(view(),
                sleHighAccount, 1, viewJ);
            uFlagsOut |= lsfHighReserve;

            if (bHigh)
                bReserveIncrease    = true;
        }

        if (bHighReserveClear && bHighReserved)
        {
            adjustOwnerCount(view(),
                sleHighAccount, -1, viewJ);
            uFlagsOut &= ~lsfHighReserve;
        }

        if (uFlagsIn != uFlagsOut)
            sleRippleState->setFieldU32 (sfFlags, uFlagsOut);

        if (bDefault || badCurrency() == currency)
        {

            terResult = trustDelete (view(),
                sleRippleState, uLowAccountID, uHighAccountID, viewJ);
        }
        else if (bReserveIncrease && mPriorBalance < reserveCreate)
        {
            JLOG(j_.trace()) <<
                "Delay transaction: Insufficent reserve to add trust line.";

            terResult = tecINSUF_RESERVE_LINE;
        }
        else
        {
            view().update (sleRippleState);

            JLOG(j_.trace()) << "Modify ripple line";
        }
    }
    else if (! saLimitAmount &&                          
        (! bQualityIn || ! uQualityIn) &&           
        (! bQualityOut || ! uQualityOut) &&         
        (! (view().rules().enabled(featureTrustSetAuth)) || ! bSetAuth))
    {
        JLOG(j_.trace()) <<
            "Redundant: Setting non-existent ripple line to defaults.";
        return tecNO_LINE_REDUNDANT;
    }
    else if (mPriorBalance < reserveCreate) 
    {
        JLOG(j_.trace()) <<
            "Delay transaction: Line does not exist. Insufficent reserve to create line.";

        terResult = tecNO_LINE_INSUF_RESERVE;
    }
    else
    {
        STAmount saBalance ({currency, noAccount()});

        uint256 index (getRippleStateIndex (
            account_, uDstAccountID, currency));

        JLOG(j_.trace()) <<
            "doTrustSet: Creating ripple line: " <<
            to_string (index);

        terResult = trustCreate (view(),
            bHigh,
            account_,
            uDstAccountID,
            index,
            sle,
            bSetAuth,
            bSetNoRipple && !bClearNoRipple,
            bSetFreeze && !bClearFreeze,
            saBalance,
            saLimitAllow,       
            uQualityIn,
            uQualityOut, viewJ);
    }

    return terResult;
}

}
























