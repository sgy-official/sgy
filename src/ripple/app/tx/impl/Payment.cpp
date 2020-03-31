

#include <ripple/app/tx/impl/Payment.h>
#include <ripple/app/paths/RippleCalc.h>
#include <ripple/basics/Log.h>
#include <ripple/core/Config.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/jss.h>
#include <ripple/protocol/st.h>
#include <ripple/protocol/TxFlags.h>

namespace ripple {


XRPAmount
Payment::calculateMaxSpend(STTx const& tx)
{
    if (tx.isFieldPresent(sfSendMax))
    {
        auto const& sendMax = tx[sfSendMax];
        return sendMax.native() ? sendMax.xrp() : beast::zero;
    }
    
    auto const& saDstAmount = tx.getFieldAmount(sfAmount);
    return saDstAmount.native() ? saDstAmount.xrp() : beast::zero;
}

NotTEC
Payment::preflight (PreflightContext const& ctx)
{
    auto const ret = preflight1 (ctx);

    if (!isTesSuccess (ret))
        return ret;

    auto& tx = ctx.tx;
    auto& j = ctx.j;

    std::uint32_t const uTxFlags = tx.getFlags ();

    if (uTxFlags & tfPaymentMask)
    {
        JLOG(j.trace()) << "Malformed transaction: " <<
            "Invalid flags set.";
        return temINVALID_FLAG;
    }

    bool const partialPaymentAllowed = uTxFlags & tfPartialPayment;
    bool const limitQuality = uTxFlags & tfLimitQuality;
    bool const defaultPathsAllowed = !(uTxFlags & tfNoRippleDirect);
    bool const bPaths = tx.isFieldPresent (sfPaths);
    bool const bMax = tx.isFieldPresent (sfSendMax);

    STAmount const saDstAmount (tx.getFieldAmount (sfAmount));

    STAmount maxSourceAmount;
    auto const account = tx.getAccountID(sfAccount);

    if (bMax)
        maxSourceAmount = tx.getFieldAmount (sfSendMax);
    else if (saDstAmount.native ())
        maxSourceAmount = saDstAmount;
    else
        maxSourceAmount = STAmount (
            { saDstAmount.getCurrency (), account },
            saDstAmount.mantissa(), saDstAmount.exponent (),
            saDstAmount < beast::zero);

    auto const& uSrcCurrency = maxSourceAmount.getCurrency ();
    auto const& uDstCurrency = saDstAmount.getCurrency ();

    bool const bXRPDirect = uSrcCurrency.isZero () && uDstCurrency.isZero ();

    if (!isLegalNet (saDstAmount) || !isLegalNet (maxSourceAmount))
        return temBAD_AMOUNT;

    auto const uDstAccountID = tx.getAccountID (sfDestination);

    if (!uDstAccountID)
    {
        JLOG(j.trace()) << "Malformed transaction: " <<
            "Payment destination account not specified.";
        return temDST_NEEDED;
    }
    if (bMax && maxSourceAmount <= beast::zero)
    {
        JLOG(j.trace()) << "Malformed transaction: " <<
            "bad max amount: " << maxSourceAmount.getFullText ();
        return temBAD_AMOUNT;
    }
    if (saDstAmount <= beast::zero)
    {
        JLOG(j.trace()) << "Malformed transaction: "<<
            "bad dst amount: " << saDstAmount.getFullText ();
        return temBAD_AMOUNT;
    }
    if (badCurrency() == uSrcCurrency || badCurrency() == uDstCurrency)
    {
        JLOG(j.trace()) <<"Malformed transaction: " <<
            "Bad currency.";
        return temBAD_CURRENCY;
    }
    if (account == uDstAccountID && uSrcCurrency == uDstCurrency && !bPaths)
    {
        JLOG(j.trace()) << "Malformed transaction: " <<
            "Redundant payment from " << to_string (account) <<
            " to self without path for " << to_string (uDstCurrency);
        return temREDUNDANT;
    }
    if (bXRPDirect && bMax)
    {
        JLOG(j.trace()) << "Malformed transaction: " <<
            "SendMax specified for XRP to XRP.";
        return temBAD_SEND_XRP_MAX;
    }
    if (bXRPDirect && bPaths)
    {
        JLOG(j.trace()) << "Malformed transaction: " <<
            "Paths specified for XRP to XRP.";
        return temBAD_SEND_XRP_PATHS;
    }
    if (bXRPDirect && partialPaymentAllowed)
    {
        JLOG(j.trace()) << "Malformed transaction: " <<
            "Partial payment specified for XRP to XRP.";
        return temBAD_SEND_XRP_PARTIAL;
    }
    if (bXRPDirect && limitQuality)
    {
        JLOG(j.trace()) << "Malformed transaction: " <<
            "Limit quality specified for XRP to XRP.";
        return temBAD_SEND_XRP_LIMIT;
    }
    if (bXRPDirect && !defaultPathsAllowed)
    {
        JLOG(j.trace()) << "Malformed transaction: " <<
            "No ripple direct specified for XRP to XRP.";
        return temBAD_SEND_XRP_NO_DIRECT;
    }

    auto const deliverMin = tx[~sfDeliverMin];
    if (deliverMin)
    {
        if (! partialPaymentAllowed)
        {
            JLOG(j.trace()) << "Malformed transaction: Partial payment not "
                "specified for " << jss::DeliverMin.c_str() << ".";
            return temBAD_AMOUNT;
        }

        auto const dMin = *deliverMin;
        if (!isLegalNet(dMin) || dMin <= beast::zero)
        {
            JLOG(j.trace()) << "Malformed transaction: Invalid " <<
                jss::DeliverMin.c_str() << " amount. " <<
                    dMin.getFullText();
            return temBAD_AMOUNT;
        }
        if (dMin.issue() != saDstAmount.issue())
        {
            JLOG(j.trace()) <<  "Malformed transaction: Dst issue differs "
                "from " << jss::DeliverMin.c_str() << ". " <<
                    dMin.getFullText();
            return temBAD_AMOUNT;
        }
        if (dMin > saDstAmount)
        {
            JLOG(j.trace()) << "Malformed transaction: Dst amount less than " <<
                jss::DeliverMin.c_str() << ". " <<
                    dMin.getFullText();
            return temBAD_AMOUNT;
        }
    }

    return preflight2 (ctx);
}

TER
Payment::preclaim(PreclaimContext const& ctx)
{
    std::uint32_t const uTxFlags = ctx.tx.getFlags();
    bool const partialPaymentAllowed = uTxFlags & tfPartialPayment;
    auto const paths = ctx.tx.isFieldPresent(sfPaths);
    auto const sendMax = ctx.tx[~sfSendMax];

    AccountID const uDstAccountID(ctx.tx[sfDestination]);
    STAmount const saDstAmount(ctx.tx[sfAmount]);

    auto const k = keylet::account(uDstAccountID);
    auto const sleDst = ctx.view.read(k);

    if (!sleDst)
    {
        if (!saDstAmount.native())
        {
            JLOG(ctx.j.trace()) <<
                "Delay transaction: Destination account does not exist.";

            return tecNO_DST;
        }
        else if (ctx.view.open()
            && partialPaymentAllowed)
        {
            JLOG(ctx.j.trace()) <<
                "Delay transaction: Partial payment not allowed to create account.";


            return telNO_DST_PARTIAL;
        }
        else if (saDstAmount < STAmount(ctx.view.fees().accountReserve(0)))
        {
            JLOG(ctx.j.trace()) <<
                "Delay transaction: Destination account does not exist. " <<
                "Insufficent payment to create account.";

            return tecNO_DST_INSUF_XRP;
        }
    }
    else if ((sleDst->getFlags() & lsfRequireDestTag) &&
        !ctx.tx.isFieldPresent(sfDestinationTag))
    {

        JLOG(ctx.j.trace()) << "Malformed transaction: DestinationTag required.";

        return tecDST_TAG_NEEDED;
    }

    if (paths || sendMax || !saDstAmount.native())
    {

        STPathSet const spsPaths = ctx.tx.getFieldPathSet(sfPaths);

        auto pathTooBig = spsPaths.size() > MaxPathSize;

        if(!pathTooBig)
            for (auto const& path : spsPaths)
                if (path.size() > MaxPathLength)
                {
                    pathTooBig = true;
                    break;
                }

        if (ctx.view.open() && pathTooBig)
        {
            return telBAD_PATH_COUNT; 
        }
    }

    return tesSUCCESS;
}


TER
Payment::doApply ()
{
    auto const deliverMin = ctx_.tx[~sfDeliverMin];

    std::uint32_t const uTxFlags = ctx_.tx.getFlags ();
    bool const partialPaymentAllowed = uTxFlags & tfPartialPayment;
    bool const limitQuality = uTxFlags & tfLimitQuality;
    bool const defaultPathsAllowed = !(uTxFlags & tfNoRippleDirect);
    auto const paths = ctx_.tx.isFieldPresent(sfPaths);
    auto const sendMax = ctx_.tx[~sfSendMax];

    AccountID const uDstAccountID (ctx_.tx.getAccountID (sfDestination));
    STAmount const saDstAmount (ctx_.tx.getFieldAmount (sfAmount));
    STAmount maxSourceAmount;
    if (sendMax)
        maxSourceAmount = *sendMax;
    else if (saDstAmount.native ())
        maxSourceAmount = saDstAmount;
    else
        maxSourceAmount = STAmount (
            {saDstAmount.getCurrency (), account_},
            saDstAmount.mantissa(), saDstAmount.exponent (),
            saDstAmount < beast::zero);

    JLOG(j_.trace()) <<
        "maxSourceAmount=" << maxSourceAmount.getFullText () <<
        " saDstAmount=" << saDstAmount.getFullText ();

    auto const k = keylet::account(uDstAccountID);
    SLE::pointer sleDst = view().peek (k);

    if (!sleDst)
    {
        sleDst = std::make_shared<SLE>(k);
        sleDst->setAccountID(sfAccount, uDstAccountID);
        sleDst->setFieldU32(sfSequence, 1);
        view().insert(sleDst);
    }
    else
    {
        view().update (sleDst);
    }

    bool const reqDepositAuth = sleDst->getFlags() & lsfDepositAuth &&
        view().rules().enabled(featureDepositAuth);

    bool const depositPreauth = view().rules().enabled(featureDepositPreauth);

    bool const bRipple = paths || sendMax || !saDstAmount.native ();

    if (!depositPreauth && bRipple && reqDepositAuth)
        return tecNO_PERMISSION;

    if (bRipple)
    {

        if (depositPreauth && reqDepositAuth)
        {
            if (uDstAccountID != account_)
            {
                if (! view().exists (
                    keylet::depositPreauth (uDstAccountID, account_)))
                    return tecNO_PERMISSION;
            }
        }

        STPathSet spsPaths = ctx_.tx.getFieldPathSet (sfPaths);

        path::RippleCalc::Input rcInput;
        rcInput.partialPaymentAllowed = partialPaymentAllowed;
        rcInput.defaultPathsAllowed = defaultPathsAllowed;
        rcInput.limitQuality = limitQuality;
        rcInput.isLedgerOpen = view().open();

        path::RippleCalc::Output rc;
        {
            PaymentSandbox pv(&view());
            JLOG(j_.debug())
                << "Entering RippleCalc in payment: "
                << ctx_.tx.getTransactionID();
            rc = path::RippleCalc::rippleCalculate (
                pv,
                maxSourceAmount,
                saDstAmount,
                uDstAccountID,
                account_,
                spsPaths,
                ctx_.app.logs(),
                &rcInput);
            pv.apply(ctx_.rawView());
        }

        if (rc.result () == tesSUCCESS &&
            rc.actualAmountOut != saDstAmount)
        {
            if (deliverMin && rc.actualAmountOut <
                *deliverMin)
                rc.setResult (tecPATH_PARTIAL);
            else
                ctx_.deliver (rc.actualAmountOut);
        }

        auto terResult = rc.result ();

        if (isTerRetry (terResult))
            terResult = tecPATH_DRY;
        return terResult;
    }

    assert (saDstAmount.native ());


    auto const uOwnerCount = view().read(
        keylet::account(account_))->getFieldU32 (sfOwnerCount);

    auto const reserve = view().fees().accountReserve(uOwnerCount);

    auto const mmm = std::max(reserve,
        ctx_.tx.getFieldAmount (sfFee).xrp ());

    if (mPriorBalance < saDstAmount.xrp () + mmm)
    {
        JLOG(j_.trace()) << "Delay transaction: Insufficient funds: " <<
            " " << to_string (mPriorBalance) <<
            " / " << to_string (saDstAmount.xrp () + mmm) <<
            " (" << to_string (reserve) << ")";

        return tecUNFUNDED_PAYMENT;
    }

    if (reqDepositAuth)
    {
        if (uDstAccountID != account_)
        {
            if (! view().exists (
                keylet::depositPreauth (uDstAccountID, account_)))
            {
                XRPAmount const dstReserve {view().fees().accountReserve (0)};

                if (saDstAmount > dstReserve ||
                    sleDst->getFieldAmount (sfBalance) > dstReserve)
                        return tecNO_PERMISSION;
            }
        }
    }

    view()
        .peek(keylet::account(account_))
        ->setFieldAmount(sfBalance, mSourceBalance - saDstAmount);
    sleDst->setFieldAmount(
        sfBalance, sleDst->getFieldAmount(sfBalance) + saDstAmount);

    if ((sleDst->getFlags() & lsfPasswordSpent))
        sleDst->clearFlag(lsfPasswordSpent);

    return tesSUCCESS;
}

}  
























