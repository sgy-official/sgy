

#include <ripple/app/tx/impl/CreateOffer.h>
#include <ripple/app/ledger/OrderBookDB.h>
#include <ripple/app/paths/Flow.h>
#include <ripple/ledger/CashDiff.h>
#include <ripple/ledger/PaymentSandbox.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/st.h>
#include <ripple/protocol/Quality.h>
#include <ripple/beast/utility/WrappedSink.h>

namespace ripple {

XRPAmount
CreateOffer::calculateMaxSpend(STTx const& tx)
{
    auto const& saTakerGets = tx[sfTakerGets];

    return saTakerGets.native() ?
        saTakerGets.xrp() : beast::zero;
}

NotTEC
CreateOffer::preflight (PreflightContext const& ctx)
{
    auto const ret = preflight1 (ctx);
    if (!isTesSuccess (ret))
        return ret;

    auto& tx = ctx.tx;
    auto& j = ctx.j;

    std::uint32_t const uTxFlags = tx.getFlags ();

    if (uTxFlags & tfOfferCreateMask)
    {
        JLOG(j.debug()) <<
            "Malformed transaction: Invalid flags set.";
        return temINVALID_FLAG;
    }

    bool const bImmediateOrCancel (uTxFlags & tfImmediateOrCancel);
    bool const bFillOrKill (uTxFlags & tfFillOrKill);

    if (bImmediateOrCancel && bFillOrKill)
    {
        JLOG(j.debug()) <<
            "Malformed transaction: both IoC and FoK set.";
        return temINVALID_FLAG;
    }

    bool const bHaveExpiration (tx.isFieldPresent (sfExpiration));

    if (bHaveExpiration && (tx.getFieldU32 (sfExpiration) == 0))
    {
        JLOG(j.debug()) <<
            "Malformed offer: bad expiration";
        return temBAD_EXPIRATION;
    }

    bool const bHaveCancel (tx.isFieldPresent (sfOfferSequence));

    if (bHaveCancel && (tx.getFieldU32 (sfOfferSequence) == 0))
    {
        JLOG(j.debug()) <<
            "Malformed offer: bad cancel sequence";
        return temBAD_SEQUENCE;
    }

    STAmount saTakerPays = tx[sfTakerPays];
    STAmount saTakerGets = tx[sfTakerGets];

    if (!isLegalNet (saTakerPays) || !isLegalNet (saTakerGets))
        return temBAD_AMOUNT;

    if (saTakerPays.native () && saTakerGets.native ())
    {
        JLOG(j.debug()) <<
            "Malformed offer: redundant (XRP for XRP)";
        return temBAD_OFFER;
    }
    if (saTakerPays <= beast::zero || saTakerGets <= beast::zero)
    {
        JLOG(j.debug()) <<
            "Malformed offer: bad amount";
        return temBAD_OFFER;
    }

    auto const& uPaysIssuerID = saTakerPays.getIssuer ();
    auto const& uPaysCurrency = saTakerPays.getCurrency ();

    auto const& uGetsIssuerID = saTakerGets.getIssuer ();
    auto const& uGetsCurrency = saTakerGets.getCurrency ();

    if (uPaysCurrency == uGetsCurrency && uPaysIssuerID == uGetsIssuerID)
    {
        JLOG(j.debug()) <<
            "Malformed offer: redundant (IOU for IOU)";
        return temREDUNDANT;
    }
    if (badCurrency() == uPaysCurrency || badCurrency() == uGetsCurrency)
    {
        JLOG(j.debug()) <<
            "Malformed offer: bad currency";
        return temBAD_CURRENCY;
    }

    if (saTakerPays.native () != !uPaysIssuerID ||
        saTakerGets.native () != !uGetsIssuerID)
    {
        JLOG(j.warn()) <<
            "Malformed offer: bad issuer";
        return temBAD_ISSUER;
    }

    return preflight2(ctx);
}

TER
CreateOffer::preclaim(PreclaimContext const& ctx)
{
    auto const id = ctx.tx[sfAccount];

    auto saTakerPays = ctx.tx[sfTakerPays];
    auto saTakerGets = ctx.tx[sfTakerGets];

    auto const& uPaysIssuerID = saTakerPays.getIssuer();
    auto const& uPaysCurrency = saTakerPays.getCurrency();

    auto const& uGetsIssuerID = saTakerGets.getIssuer();

    auto const cancelSequence = ctx.tx[~sfOfferSequence];

    auto const sleCreator = ctx.view.read(keylet::account(id));

    std::uint32_t const uAccountSequence = sleCreator->getFieldU32(sfSequence);

    auto viewJ = ctx.app.journal("View");

    if (isGlobalFrozen(ctx.view, uPaysIssuerID) ||
        isGlobalFrozen(ctx.view, uGetsIssuerID))
    {
        JLOG(ctx.j.info()) <<
            "Offer involves frozen asset";

        return tecFROZEN;
    }
    else if (accountFunds(ctx.view, id, saTakerGets,
        fhZERO_IF_FROZEN, viewJ) <= beast::zero)
    {
        JLOG(ctx.j.debug()) <<
            "delay: Offers must be at least partially funded.";

        return tecUNFUNDED_OFFER;
    }
    else if (cancelSequence && (uAccountSequence <= *cancelSequence))
    {
        JLOG(ctx.j.debug()) <<
            "uAccountSequenceNext=" << uAccountSequence <<
            " uOfferSequence=" << *cancelSequence;

        return temBAD_SEQUENCE;
    }

    using d = NetClock::duration;
    using tp = NetClock::time_point;
    auto const expiration = ctx.tx[~sfExpiration];

    if (expiration &&
        (ctx.view.parentCloseTime() >= tp{d{*expiration}}))
    {
        return ctx.view.rules().enabled(featureDepositPreauth)
            ? TER {tecEXPIRED}
            : TER {tesSUCCESS};
    }

    if (!saTakerPays.native())
    {
        auto result = checkAcceptAsset(ctx.view, ctx.flags,
            id, ctx.j, Issue(uPaysCurrency, uPaysIssuerID));
        if (result != tesSUCCESS)
            return result;
    }

    return tesSUCCESS;
}

TER
CreateOffer::checkAcceptAsset(ReadView const& view,
    ApplyFlags const flags, AccountID const id,
        beast::Journal const j, Issue const& issue)
{
    assert (!isXRP (issue.currency));

    auto const issuerAccount = view.read(
        keylet::account(issue.account));

    if (!issuerAccount)
    {
        JLOG(j.warn()) <<
            "delay: can't receive IOUs from non-existent issuer: " <<
            to_string (issue.account);

        return (flags & tapRETRY)
            ? TER {terNO_ACCOUNT}
            : TER {tecNO_ISSUER};
    }

    if (view.rules().enabled(featureDepositPreauth) && (issue.account == id))
        return tesSUCCESS;

    if ((*issuerAccount)[sfFlags] & lsfRequireAuth)
    {
        auto const trustLine = view.read(
            keylet::line(id, issue.account, issue.currency));

        if (!trustLine)
        {
            return (flags & tapRETRY)
                ? TER {terNO_LINE}
                : TER {tecNO_LINE};
        }

        bool const canonical_gt (id > issue.account);

        bool const is_authorized ((*trustLine)[sfFlags] &
            (canonical_gt ? lsfLowAuth : lsfHighAuth));

        if (!is_authorized)
        {
            JLOG(j.debug()) <<
                "delay: can't receive IOUs from issuer without auth.";

            return (flags & tapRETRY)
                ? TER {terNO_AUTH}
                : TER {tecNO_AUTH};
        }
    }

    return tesSUCCESS;
}

bool
CreateOffer::dry_offer (ApplyView& view, Offer const& offer)
{
    if (offer.fully_consumed ())
        return true;
    auto const amount = accountFunds(view, offer.owner(),
        offer.amount().out, fhZERO_IF_FROZEN, ctx_.app.journal ("View"));
    return (amount <= beast::zero);
}

std::pair<bool, Quality>
CreateOffer::select_path (
    bool have_direct, OfferStream const& direct,
    bool have_bridge, OfferStream const& leg1, OfferStream const& leg2)
{
    assert (have_direct || have_bridge);

    if (!have_bridge)
        return std::make_pair (true, direct.tip ().quality ());

    Quality const bridged_quality (composed_quality (
        leg1.tip ().quality (), leg2.tip ().quality ()));

    if (have_direct)
    {
        Quality const direct_quality (direct.tip ().quality ());

        if (bridged_quality < direct_quality)
            return std::make_pair (true, direct_quality);
    }

    return std::make_pair (false, bridged_quality);
}

bool
CreateOffer::reachedOfferCrossingLimit (Taker const& taker) const
{
    auto const crossings =
        taker.get_direct_crossings () +
        (2 * taker.get_bridge_crossings ());

    return crossings >= 850;
}

std::pair<TER, Amounts>
CreateOffer::bridged_cross (
    Taker& taker,
    ApplyView& view,
    ApplyView& view_cancel,
    NetClock::time_point const when)
{
    auto const& takerAmount = taker.original_offer ();

    assert (!isXRP (takerAmount.in) && !isXRP (takerAmount.out));

    if (isXRP (takerAmount.in) || isXRP (takerAmount.out))
        Throw<std::logic_error> ("Bridging with XRP and an endpoint.");

    OfferStream offers_direct (view, view_cancel,
        Book (taker.issue_in (), taker.issue_out ()),
            when, stepCounter_, j_);

    OfferStream offers_leg1 (view, view_cancel,
        Book (taker.issue_in (), xrpIssue ()),
        when, stepCounter_, j_);

    OfferStream offers_leg2 (view, view_cancel,
        Book (xrpIssue (), taker.issue_out ()),
        when, stepCounter_, j_);

    TER cross_result = tesSUCCESS;

    bool have_bridge = offers_leg1.step () && offers_leg2.step ();
    bool have_direct = step_account (offers_direct, taker);
    int count = 0;

    auto viewJ = ctx_.app.journal ("View");

    while (have_direct || have_bridge)
    {
        bool leg1_consumed = false;
        bool leg2_consumed = false;
        bool direct_consumed = false;

        Quality quality;
        bool use_direct;

        std::tie (use_direct, quality) = select_path (
            have_direct, offers_direct,
            have_bridge, offers_leg1, offers_leg2);


        if (taker.reject(quality))
            break;

        count++;

        if (use_direct)
        {
            if (auto stream = j_.debug())
            {
                stream << count << " Direct:";
                stream << "  offer: " << offers_direct.tip ();
                stream << "     in: " << offers_direct.tip ().amount().in;
                stream << "    out: " << offers_direct.tip ().amount ().out;
                stream << "  owner: " << offers_direct.tip ().owner ();
                stream << "  funds: " << accountFunds(view,
                    offers_direct.tip ().owner (),
                    offers_direct.tip ().amount ().out,
                    fhIGNORE_FREEZE, viewJ);
            }

            cross_result = taker.cross(offers_direct.tip ());

            JLOG (j_.debug()) << "Direct Result: " << transToken (cross_result);

            if (dry_offer (view, offers_direct.tip ()))
            {
                direct_consumed = true;
                have_direct = step_account (offers_direct, taker);
            }
        }
        else
        {
            if (auto stream = j_.debug())
            {
                auto const owner1_funds_before = accountFunds(view,
                    offers_leg1.tip ().owner (),
                    offers_leg1.tip ().amount ().out,
                    fhIGNORE_FREEZE, viewJ);

                auto const owner2_funds_before = accountFunds(view,
                    offers_leg2.tip ().owner (),
                    offers_leg2.tip ().amount ().out,
                    fhIGNORE_FREEZE, viewJ);

                stream << count << " Bridge:";
                stream << " offer1: " << offers_leg1.tip ();
                stream << "     in: " << offers_leg1.tip ().amount().in;
                stream << "    out: " << offers_leg1.tip ().amount ().out;
                stream << "  owner: " << offers_leg1.tip ().owner ();
                stream << "  funds: " << owner1_funds_before;
                stream << " offer2: " << offers_leg2.tip ();
                stream << "     in: " << offers_leg2.tip ().amount ().in;
                stream << "    out: " << offers_leg2.tip ().amount ().out;
                stream << "  owner: " << offers_leg2.tip ().owner ();
                stream << "  funds: " << owner2_funds_before;
            }

            cross_result = taker.cross(offers_leg1.tip (), offers_leg2.tip ());

            JLOG (j_.debug()) << "Bridge Result: " << transToken (cross_result);

            if (view.rules().enabled (fixTakerDryOfferRemoval))
            {
                leg1_consumed = dry_offer (view, offers_leg1.tip());
                if (leg1_consumed)
                    have_bridge &= offers_leg1.step();

                leg2_consumed = dry_offer (view, offers_leg2.tip());
                if (leg2_consumed)
                    have_bridge &= offers_leg2.step();
            }
            else
            {
                if (dry_offer (view, offers_leg1.tip ()))
                {
                    leg1_consumed = true;
                    have_bridge = (have_bridge && offers_leg1.step ());
                }
                if (dry_offer (view, offers_leg2.tip ()))
                {
                    leg2_consumed = true;
                    have_bridge = (have_bridge && offers_leg2.step ());
                }
            }
        }

        if (cross_result != tesSUCCESS)
        {
            cross_result = tecFAILED_PROCESSING;
            break;
        }

        if (taker.done())
        {
            JLOG(j_.debug()) << "The taker reports he's done during crossing!";
            break;
        }

        if (reachedOfferCrossingLimit (taker))
        {
            JLOG(j_.debug()) << "The offer crossing limit has been exceeded!";
            break;
        }

        assert (direct_consumed || leg1_consumed || leg2_consumed);

        if (!direct_consumed && !leg1_consumed && !leg2_consumed)
            Throw<std::logic_error> ("bridged crossing: nothing was fully consumed.");
    }

    return std::make_pair(cross_result, taker.remaining_offer ());
}

std::pair<TER, Amounts>
CreateOffer::direct_cross (
    Taker& taker,
    ApplyView& view,
    ApplyView& view_cancel,
    NetClock::time_point const when)
{
    OfferStream offers (
        view, view_cancel,
        Book (taker.issue_in (), taker.issue_out ()),
        when, stepCounter_, j_);

    TER cross_result (tesSUCCESS);
    int count = 0;

    bool have_offer = step_account (offers, taker);

    while (have_offer)
    {
        bool direct_consumed = false;
        auto& offer (offers.tip());

        if (taker.reject (offer.quality()))
            break;

        count++;

        if (auto stream = j_.debug())
        {
            stream << count << " Direct:";
            stream << "  offer: " << offer;
            stream << "     in: " << offer.amount ().in;
            stream << "    out: " << offer.amount ().out;
            stream << "quality: " << offer.quality();
            stream << "  owner: " << offer.owner ();
            stream << "  funds: " << accountFunds(view,
                offer.owner (), offer.amount ().out, fhIGNORE_FREEZE,
                ctx_.app.journal ("View"));
        }

        cross_result = taker.cross (offer);

        JLOG (j_.debug()) << "Direct Result: " << transToken (cross_result);

        if (dry_offer (view, offer))
        {
            direct_consumed = true;
            have_offer = step_account (offers, taker);
        }

        if (cross_result != tesSUCCESS)
        {
            cross_result = tecFAILED_PROCESSING;
            break;
        }

        if (taker.done())
        {
            JLOG(j_.debug()) << "The taker reports he's done during crossing!";
            break;
        }

        if (reachedOfferCrossingLimit (taker))
        {
            JLOG(j_.debug()) << "The offer crossing limit has been exceeded!";
            break;
        }

        assert (direct_consumed);

        if (!direct_consumed)
            Throw<std::logic_error> ("direct crossing: nothing was fully consumed.");
    }

    return std::make_pair(cross_result, taker.remaining_offer ());
}

bool
CreateOffer::step_account (OfferStream& stream, Taker const& taker)
{
    while (stream.step ())
    {
        auto const& offer = stream.tip ();

        if (taker.reject (offer.quality ()))
            return true;

        if (offer.owner () != taker.account ())
            return true;
    }

    return false;
}

std::pair<TER, Amounts>
CreateOffer::takerCross (
    Sandbox& sb,
    Sandbox& sbCancel,
    Amounts const& takerAmount)
{
    NetClock::time_point const when{ctx_.view().parentCloseTime()};

    beast::WrappedSink takerSink (j_, "Taker ");

    Taker taker (cross_type_, sb, account_, takerAmount,
        ctx_.tx.getFlags(), beast::Journal (takerSink));

    if (taker.unfunded ())
    {
        JLOG (j_.debug()) <<
            "Not crossing: taker is unfunded.";
        return { tecUNFUNDED_OFFER, takerAmount };
    }

    try
    {
        if (cross_type_ == CrossType::IouToIou)
            return bridged_cross (taker, sb, sbCancel, when);

        return direct_cross (taker, sb, sbCancel, when);
    }
    catch (std::exception const& e)
    {
        JLOG (j_.error()) <<
            "Exception during offer crossing: " << e.what ();
        return { tecINTERNAL, taker.remaining_offer () };
    }
}

std::pair<TER, Amounts>
CreateOffer::flowCross (
    PaymentSandbox& psb,
    PaymentSandbox& psbCancel,
    Amounts const& takerAmount)
{
    try
    {
        STAmount const inStartBalance = accountFunds (
            psb, account_, takerAmount.in, fhZERO_IF_FROZEN, j_);
        if (inStartBalance <= beast::zero)
        {
            JLOG (j_.debug()) <<
                "Not crossing: taker is unfunded.";
            return { tecUNFUNDED_OFFER, takerAmount };
        }

        Rate gatewayXferRate {QUALITY_ONE};
        STAmount sendMax = takerAmount.in;
        if (! sendMax.native() && (account_ != sendMax.getIssuer()))
        {
            gatewayXferRate = transferRate (psb, sendMax.getIssuer());
            if (gatewayXferRate.value != QUALITY_ONE)
            {
                sendMax = multiplyRound (takerAmount.in,
                    gatewayXferRate, takerAmount.in.issue(), true);
            }
        }

        Quality threshold { takerAmount.out, sendMax };

        std::uint32_t const txFlags = ctx_.tx.getFlags();
        if (txFlags & tfPassive)
            ++threshold;

        if (sendMax > inStartBalance)
            sendMax = inStartBalance;

        STPathSet paths;
        if (!takerAmount.in.native() & !takerAmount.out.native())
        {
            STPath path;
            path.emplace_back (boost::none, xrpCurrency(), boost::none);
            paths.emplace_back (std::move(path));
        }
        STAmount deliver = takerAmount.out;
        if (txFlags & tfSell)
        {
            if (deliver.native())
                deliver = STAmount { STAmount::cMaxNative };
            else
                deliver = STAmount { takerAmount.out.issue(),
                    STAmount::cMaxValue / 2, STAmount::cMaxOffset };
        }

        auto const result = flow (psb, deliver, account_, account_,
            paths,
            true,                       
            ! (txFlags & tfFillOrKill), 
            true,                       
            true,                       
            threshold,
            sendMax, j_);

        for (auto const& toRemove : result.removableOffers)
        {
            if (auto otr = psb.peek (keylet::offer (toRemove)))
                offerDelete (psb, otr, j_);
            if (auto otr = psbCancel.peek (keylet::offer (toRemove)))
                offerDelete (psbCancel, otr, j_);
        }

        auto afterCross = takerAmount; 
        if (isTesSuccess (result.result()))
        {
            STAmount const takerInBalance = accountFunds (
                psb, account_, takerAmount.in, fhZERO_IF_FROZEN, j_);

            if (takerInBalance <= beast::zero)
            {
                afterCross.in.clear();
                afterCross.out.clear();
            }
            else
            {
                STAmount const rate {
                    Quality{takerAmount.out, takerAmount.in}.rate() };

                if (txFlags & tfSell)
                {

                    STAmount nonGatewayAmountIn = result.actualAmountIn;
                    if (gatewayXferRate.value != QUALITY_ONE)
                        nonGatewayAmountIn = divideRound (result.actualAmountIn,
                            gatewayXferRate, takerAmount.in.issue(), true);

                    afterCross.in -= nonGatewayAmountIn;

                    if (afterCross.in < beast::zero)
                        afterCross.in.clear();

                    afterCross.out = divRound (afterCross.in,
                        rate, takerAmount.out.issue(), true);
                }
                else
                {
                    afterCross.out -= result.actualAmountOut;
                    assert (afterCross.out >= beast::zero);
                    if (afterCross.out < beast::zero)
                        afterCross.out.clear();
                    afterCross.in = mulRound (afterCross.out,
                        rate, takerAmount.in.issue(), true);
                }
            }
        }

        return { tesSUCCESS, afterCross };
    }
    catch (std::exception const& e)
    {
        JLOG (j_.error()) <<
            "Exception during offer crossing: " << e.what ();
    }
    return { tecINTERNAL, takerAmount };
}

enum class SBoxCmp
{
    same,
    dustDiff,
    offerDelDiff,
    xrpRound,
    diff
};

static std::string to_string (SBoxCmp c)
{
    switch (c)
    {
    case SBoxCmp::same:
        return "same";
    case SBoxCmp::dustDiff:
        return "dust diffs";
    case SBoxCmp::offerDelDiff:
        return "offer del diffs";
    case SBoxCmp::xrpRound:
        return "XRP round to zero";
    case SBoxCmp::diff:
        return "different";
    }
    return {};
}

static SBoxCmp compareSandboxes (char const* name, ApplyContext const& ctx,
    detail::ApplyViewBase const& viewTaker, detail::ApplyViewBase const& viewFlow,
    beast::Journal j)
{
    SBoxCmp c = SBoxCmp::same;
    CashDiff diff = cashFlowDiff (
        CashFilter::treatZeroOfferAsDeletion, viewTaker,
        CashFilter::none, viewFlow);

    if (diff.hasDiff())
    {
        using namespace beast::severities;
        if (int const side = diff.xrpRoundToZero())
        {
            char const* const whichSide = side > 0 ? "; Flow" : "; Taker";
            j.stream (kWarning) << "FlowCross: " << name << " different" <<
                whichSide << " XRP rounded to zero.  tx: " <<
                ctx.tx.getTransactionID();
            return SBoxCmp::xrpRound;
        }

        c = SBoxCmp::dustDiff;
        Severity s = kInfo;
        std::string diffDesc = ", but only dust.";
        diff.rmDust();
        if (diff.hasDiff())
        {
            std::stringstream txIdSs;
            txIdSs << ".  tx: " << ctx.tx.getTransactionID();
            auto txID = txIdSs.str();

            c = SBoxCmp::offerDelDiff;
            s = kWarning;
            int sides = diff.rmLhsDeletedOffers() ? 1 : 0;
            sides    |= diff.rmRhsDeletedOffers() ? 2 : 0;
            if (!diff.hasDiff())
            {
                char const* t = "";
                switch (sides)
                {
                case 1: t = "; Taker deleted more offers"; break;
                case 2: t = "; Flow deleted more offers"; break;
                case 3: t = "; Taker and Flow deleted different offers"; break;
                default: break;
                }
                diffDesc = std::string(t) + txID;
            }
            else
            {
                c = SBoxCmp::diff;
                std::stringstream ss;
                ss << "; common entries: " << diff.commonCount()
                    << "; Taker unique: " << diff.lhsOnlyCount()
                    << "; Flow unique: " << diff.rhsOnlyCount() << txID;
                diffDesc = ss.str();
            }
        }
        j.stream (s) << "FlowCross: " << name << " different" << diffDesc;
    }
    return c;
}

std::pair<TER, Amounts>
CreateOffer::cross (
    Sandbox& sb,
    Sandbox& sbCancel,
    Amounts const& takerAmount)
{
    using beast::zero;

    bool const useFlowCross { sb.rules().enabled (featureFlowCross) };
    bool const doCompare { sb.rules().enabled (featureCompareTakerFlowCross) };

    Sandbox sbTaker { &sb };
    Sandbox sbCancelTaker { &sbCancel };
    auto const takerR = (!useFlowCross || doCompare)
        ? takerCross (sbTaker, sbCancelTaker, takerAmount)
        : std::make_pair (tecINTERNAL, takerAmount);

    PaymentSandbox psbFlow { &sb };
    PaymentSandbox psbCancelFlow { &sbCancel };
    auto const flowR = (useFlowCross || doCompare)
        ? flowCross (psbFlow, psbCancelFlow, takerAmount)
        : std::make_pair (tecINTERNAL, takerAmount);

    if (doCompare)
    {
        SBoxCmp c = SBoxCmp::same;
        if (takerR.first != flowR.first)
        {
            c = SBoxCmp::diff;
            j_.warn() << "FlowCross: Offer cross tec codes different.  tx: "
                << ctx_.tx.getTransactionID();
        }
        else if ((takerR.second.in  == zero && flowR.second.in  == zero) ||
           (takerR.second.out == zero && flowR.second.out == zero))
        {
            c = compareSandboxes ("Both Taker and Flow fully crossed",
                ctx_, sbTaker, psbFlow, j_);
        }
        else if (takerR.second.in == zero && takerR.second.out == zero)
        {
            char const * crossType = "Taker fully crossed, Flow partially crossed";
            if (flowR.second.in == takerAmount.in &&
                flowR.second.out == takerAmount.out)
                    crossType = "Taker fully crossed, Flow not crossed";

            c = compareSandboxes (crossType, ctx_, sbTaker, psbFlow, j_);
        }
        else if (flowR.second.in == zero && flowR.second.out == zero)
        {
            char const * crossType =
                "Taker partially crossed, Flow fully crossed";
            if (takerR.second.in == takerAmount.in &&
                takerR.second.out == takerAmount.out)
                    crossType = "Taker not crossed, Flow fully crossed";

            c = compareSandboxes (crossType, ctx_, sbTaker, psbFlow, j_);
        }
        else if (ctx_.tx.getFlags() & tfFillOrKill)
        {
            c = compareSandboxes (
                "FillOrKill offer", ctx_, sbCancelTaker, psbCancelFlow, j_);
        }
        else if (takerR.second.in  == takerAmount.in &&
                 flowR.second.in   == takerAmount.in &&
                 takerR.second.out == takerAmount.out &&
                 flowR.second.out  == takerAmount.out)
        {
            char const * crossType = "Neither Taker nor Flow crossed";
            c = compareSandboxes (crossType, ctx_, sbTaker, psbFlow, j_);
        }
        else if (takerR.second.in == takerAmount.in &&
                 takerR.second.out == takerAmount.out)
        {
            char const * crossType = "Taker not crossed, Flow partially crossed";
            c = compareSandboxes (crossType, ctx_, sbTaker, psbFlow, j_);
        }
        else if (flowR.second.in == takerAmount.in &&
                 flowR.second.out == takerAmount.out)
        {
            char const * crossType = "Taker partially crossed, Flow not crossed";
            c = compareSandboxes (crossType, ctx_, sbTaker, psbFlow, j_);
        }
        else
        {
            c = compareSandboxes (
                "Partial cross offer", ctx_, sbTaker, psbFlow, j_);

            if (c <= SBoxCmp::dustDiff && takerR.second != flowR.second)
            {
                c = SBoxCmp::dustDiff;
                using namespace beast::severities;
                Severity s = kInfo;
                std::string onlyDust = ", but only dust.";
                if (! diffIsDust (takerR.second.in, flowR.second.in) ||
                    (! diffIsDust (takerR.second.out, flowR.second.out)))
                {
                    char const* outSame = "";
                    if (takerR.second.out == flowR.second.out)
                        outSame = " but outs same";

                    c = SBoxCmp::diff;
                    s = kWarning;
                    std::stringstream ss;
                    ss << outSame
                        << ".  Taker in: " << takerR.second.in.getText()
                        << "; Taker out: " << takerR.second.out.getText()
                        << "; Flow in: " << flowR.second.in.getText()
                        << "; Flow out: " << flowR.second.out.getText()
                        << ".  tx: " << ctx_.tx.getTransactionID();
                    onlyDust = ss.str();
                }
                j_.stream (s) << "FlowCross: Partial cross amounts different"
                    << onlyDust;
            }
        }
        j_.error() << "FlowCross cmp result: " << to_string (c);
    }

    if (useFlowCross)
    {
        psbFlow.apply (sb);
        psbCancelFlow.apply (sbCancel);
        return flowR;
    }

    sbTaker.apply (sb);
    sbCancelTaker.apply (sbCancel);
    return takerR;
}

std::string
CreateOffer::format_amount (STAmount const& amount)
{
    std::string txt = amount.getText ();
    txt += "/";
    txt += to_string (amount.issue().currency);
    return txt;
}

void
CreateOffer::preCompute()
{
    cross_type_ = CrossType::IouToIou;
    bool const pays_xrp =
        ctx_.tx.getFieldAmount (sfTakerPays).native ();
    bool const gets_xrp =
        ctx_.tx.getFieldAmount (sfTakerGets).native ();
    if (pays_xrp && !gets_xrp)
        cross_type_ = CrossType::IouToXrp;
    else if (gets_xrp && !pays_xrp)
        cross_type_ = CrossType::XrpToIou;

    return Transactor::preCompute();
}

std::pair<TER, bool>
CreateOffer::applyGuts (Sandbox& sb, Sandbox& sbCancel)
{
    using beast::zero;

    std::uint32_t const uTxFlags = ctx_.tx.getFlags ();

    bool const bPassive (uTxFlags & tfPassive);
    bool const bImmediateOrCancel (uTxFlags & tfImmediateOrCancel);
    bool const bFillOrKill (uTxFlags & tfFillOrKill);
    bool const bSell (uTxFlags & tfSell);

    auto saTakerPays = ctx_.tx[sfTakerPays];
    auto saTakerGets = ctx_.tx[sfTakerGets];

    auto const cancelSequence = ctx_.tx[~sfOfferSequence];

    auto const uSequence = ctx_.tx.getSequence ();

    auto uRate = getRate (saTakerGets, saTakerPays);

    auto viewJ = ctx_.app.journal("View");

    TER result = tesSUCCESS;

    if (cancelSequence)
    {
        auto const sleCancel = sb.peek(
            keylet::offer(account_, *cancelSequence));

        if (sleCancel)
        {
            JLOG(j_.debug()) << "Create cancels order " << *cancelSequence;
            result = offerDelete (sb, sleCancel, viewJ);
        }
    }

    auto const expiration = ctx_.tx[~sfExpiration];
    using d = NetClock::duration;
    using tp = NetClock::time_point;

    if (expiration &&
        (ctx_.view().parentCloseTime() >= tp{d{*expiration}}))
    {
        TER const ter {ctx_.view().rules().enabled(
            featureDepositPreauth) ? TER {tecEXPIRED} : TER {tesSUCCESS}};
        return{ ter, true };
    }

    bool const bOpenLedger = ctx_.view().open();
    bool crossed = false;

    if (result == tesSUCCESS)
    {
        auto const& uPaysIssuerID = saTakerPays.getIssuer ();
        auto const& uGetsIssuerID = saTakerGets.getIssuer ();

        std::uint8_t uTickSize = Quality::maxTickSize;
        if (!isXRP (uPaysIssuerID))
        {
            auto const sle =
                sb.read(keylet::account(uPaysIssuerID));
            if (sle && sle->isFieldPresent (sfTickSize))
                uTickSize = std::min (uTickSize,
                    (*sle)[sfTickSize]);
        }
        if (!isXRP (uGetsIssuerID))
        {
            auto const sle =
                sb.read(keylet::account(uGetsIssuerID));
            if (sle && sle->isFieldPresent (sfTickSize))
                uTickSize = std::min (uTickSize,
                    (*sle)[sfTickSize]);
        }
        if (uTickSize < Quality::maxTickSize)
        {
            auto const rate =
                Quality{saTakerGets, saTakerPays}.round
                    (uTickSize).rate();

            if (bSell)
            {
                saTakerPays = multiply (
                    saTakerGets, rate, saTakerPays.issue());
            }
            else
            {
                saTakerGets = divide (
                    saTakerPays, rate, saTakerGets.issue());
            }
            if (! saTakerGets || ! saTakerPays)
            {
                JLOG (j_.debug()) <<
                    "Offer rounded to zero";
                return { result, true };
            }

            uRate = getRate (saTakerGets, saTakerPays);
        }

        Amounts const takerAmount (saTakerGets, saTakerPays);

        Amounts place_offer;

        JLOG(j_.debug()) << "Attempting cross: " <<
            to_string (takerAmount.in.issue ()) << " -> " <<
            to_string (takerAmount.out.issue ());

        if (auto stream = j_.trace())
        {
            stream << "   mode: " <<
                (bPassive ? "passive " : "") <<
                (bSell ? "sell" : "buy");
            stream <<"     in: " << format_amount (takerAmount.in);
            stream << "    out: " << format_amount (takerAmount.out);
        }

        std::tie(result, place_offer) = cross (sb, sbCancel, takerAmount);

        assert(result == tesSUCCESS || isTecClaim(result));

        if (auto stream = j_.trace())
        {
            stream << "Cross result: " << transToken (result);
            stream << "     in: " << format_amount (place_offer.in);
            stream << "    out: " << format_amount (place_offer.out);
        }

        if (result == tecFAILED_PROCESSING && bOpenLedger)
            result = telFAILED_PROCESSING;

        if (result != tesSUCCESS)
        {
            JLOG (j_.debug()) << "final result: " << transToken (result);
            return { result, true };
        }

        assert (saTakerGets.issue () == place_offer.in.issue ());
        assert (saTakerPays.issue () == place_offer.out.issue ());

        if (takerAmount != place_offer)
            crossed = true;

        if (place_offer.in < zero || place_offer.out < zero)
        {
            JLOG(j_.fatal()) << "Cross left offer negative!" <<
                "     in: " << format_amount (place_offer.in) <<
                "    out: " << format_amount (place_offer.out);
            return { tefINTERNAL, true };
        }

        if (place_offer.in == zero || place_offer.out == zero)
        {
            JLOG(j_.debug()) << "Offer fully crossed!";
            return { result, true };
        }

        saTakerPays = place_offer.out;
        saTakerGets = place_offer.in;
    }

    assert (saTakerPays > zero && saTakerGets > zero);

    if (result != tesSUCCESS)
    {
        JLOG (j_.debug()) << "final result: " << transToken (result);
        return { result, true };
    }

    if (auto stream = j_.trace())
    {
        stream << "Place" << (crossed ? " remaining " : " ") << "offer:";
        stream << "    Pays: " << saTakerPays.getFullText ();
        stream << "    Gets: " << saTakerGets.getFullText ();
    }

    if (bFillOrKill)
    {
        JLOG (j_.trace()) << "Fill or Kill: offer killed";
        if (sb.rules().enabled (fix1578))
            return { tecKILLED, false };
        return { tesSUCCESS, false };
    }

    if (bImmediateOrCancel)
    {
        JLOG (j_.trace()) << "Immediate or cancel: offer canceled";
        return { tesSUCCESS, true };
    }

    auto const sleCreator = sb.peek (keylet::account(account_));
    {
        XRPAmount reserve = ctx_.view().fees().accountReserve(
            sleCreator->getFieldU32 (sfOwnerCount) + 1);

        if (mPriorBalance < reserve)
        {
            if (!crossed)
                result = tecINSUF_RESERVE_OFFER;

            if (result != tesSUCCESS)
            {
                JLOG (j_.debug()) <<
                    "final result: " << transToken (result);
            }

            return { result, true };
        }
    }

    auto const offer_index = getOfferIndex (account_, uSequence);

    auto const ownerNode = dirAdd(sb, keylet::ownerDir (account_),
        offer_index, false, describeOwnerDir (account_), viewJ);

    if (!ownerNode)
    {
        JLOG (j_.debug()) <<
            "final result: failed to add offer to owner's directory";
        return { tecDIR_FULL, true };
    }

    adjustOwnerCount(sb, sleCreator, 1, viewJ);

    JLOG (j_.trace()) <<
        "adding to book: " << to_string (saTakerPays.issue ()) <<
        " : " << to_string (saTakerGets.issue ());

    Book const book { saTakerPays.issue(), saTakerGets.issue() };

    auto dir = keylet::quality (keylet::book (book), uRate);
    bool const bookExisted = static_cast<bool>(sb.peek (dir));

    auto const bookNode = dirAdd (sb, dir, offer_index, true,
        [&](SLE::ref sle)
        {
            sle->setFieldH160 (sfTakerPaysCurrency,
                saTakerPays.issue().currency);
            sle->setFieldH160 (sfTakerPaysIssuer,
                saTakerPays.issue().account);
            sle->setFieldH160 (sfTakerGetsCurrency,
                saTakerGets.issue().currency);
            sle->setFieldH160 (sfTakerGetsIssuer,
                saTakerGets.issue().account);
            sle->setFieldU64 (sfExchangeRate, uRate);
        }, viewJ);

    if (!bookNode)
    {
        JLOG (j_.debug()) <<
            "final result: failed to add offer to book";
        return { tecDIR_FULL, true };
    }

    auto sleOffer = std::make_shared<SLE>(ltOFFER, offer_index);
    sleOffer->setAccountID (sfAccount, account_);
    sleOffer->setFieldU32 (sfSequence, uSequence);
    sleOffer->setFieldH256 (sfBookDirectory, dir.key);
    sleOffer->setFieldAmount (sfTakerPays, saTakerPays);
    sleOffer->setFieldAmount (sfTakerGets, saTakerGets);
    sleOffer->setFieldU64 (sfOwnerNode, *ownerNode);
    sleOffer->setFieldU64 (sfBookNode, *bookNode);
    if (expiration)
        sleOffer->setFieldU32 (sfExpiration, *expiration);
    if (bPassive)
        sleOffer->setFlag (lsfPassive);
    if (bSell)
        sleOffer->setFlag (lsfSell);
    sb.insert(sleOffer);

    if (!bookExisted)
        ctx_.app.getOrderBookDB().addOrderBook(book);

    JLOG (j_.debug()) << "final result: success";

    return { tesSUCCESS, true };
}

TER
CreateOffer::doApply()
{
    Sandbox sb (&ctx_.view());

    Sandbox sbCancel (&ctx_.view());

    auto const result = applyGuts(sb, sbCancel);
    if (result.second)
        sb.apply(ctx_.rawView());
    else
        sbCancel.apply(ctx_.rawView());
    return result.first;
}

}
























