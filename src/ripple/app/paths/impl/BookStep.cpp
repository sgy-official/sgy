

#include <ripple/app/paths/impl/FlatSets.h>
#include <ripple/app/paths/impl/Steps.h>
#include <ripple/app/paths/Credit.h>
#include <ripple/app/paths/NodeDirectory.h>
#include <ripple/app/tx/impl/OfferStream.h>
#include <ripple/basics/contract.h>
#include <ripple/basics/Log.h>
#include <ripple/ledger/Directory.h>
#include <ripple/ledger/PaymentSandbox.h>
#include <ripple/protocol/Book.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/IOUAmount.h>
#include <ripple/protocol/Quality.h>
#include <ripple/protocol/XRPAmount.h>

#include <boost/container/flat_set.hpp>

#include <numeric>
#include <sstream>

namespace ripple {

template<class TIn, class TOut, class TDerived>
class BookStep : public StepImp<TIn, TOut, BookStep<TIn, TOut, TDerived>>
{
protected:
    uint32_t const maxOffersToConsume_;
    Book book_;
    AccountID strandSrc_;
    AccountID strandDst_;
    Step const* const prevStep_ = nullptr;
    bool const ownerPaysTransferFee_;
    bool inactive_ = false;
    beast::Journal j_;

    struct Cache
    {
        TIn in;
        TOut out;

        Cache (TIn const& in_, TOut const& out_)
            : in (in_), out (out_)
        {
        }
    };

    boost::optional<Cache> cache_;

    static
    uint32_t getMaxOffersToConsume(StrandContext const& ctx)
    {
        if (ctx.view.rules().enabled(fix1515))
            return 1000;
        return 2000;
    }

public:
    BookStep (StrandContext const& ctx,
        Issue const& in,
        Issue const& out)
        : maxOffersToConsume_ (getMaxOffersToConsume(ctx))
        , book_ (in, out)
        , strandSrc_ (ctx.strandSrc)
        , strandDst_ (ctx.strandDst)
        , prevStep_ (ctx.prevStep)
        , ownerPaysTransferFee_ (ctx.ownerPaysTransferFee)
        , j_ (ctx.j)
    {
    }

    Book const& book() const
    {
        return book_;
    }

    boost::optional<EitherAmount>
    cachedIn () const override
    {
        if (!cache_)
            return boost::none;
        return EitherAmount (cache_->in);
    }

    boost::optional<EitherAmount>
    cachedOut () const override
    {
        if (!cache_)
            return boost::none;
        return EitherAmount (cache_->out);
    }

    bool
    redeems (ReadView const& sb, bool fwd) const override
    {
        return !ownerPaysTransferFee_;
    }

    boost::optional<Book>
    bookStepBook () const override
    {
        return book_;
    }

    boost::optional<Quality>
    qualityUpperBound(ReadView const& v, bool& redeems) const override;

    std::pair<TIn, TOut>
    revImp (
        PaymentSandbox& sb,
        ApplyView& afView,
        boost::container::flat_set<uint256>& ofrsToRm,
        TOut const& out);

    std::pair<TIn, TOut>
    fwdImp (
        PaymentSandbox& sb,
        ApplyView& afView,
        boost::container::flat_set<uint256>& ofrsToRm,
        TIn const& in);

    std::pair<bool, EitherAmount>
    validFwd (
        PaymentSandbox& sb,
        ApplyView& afView,
        EitherAmount const& in) override;

    TER check(StrandContext const& ctx) const;

    bool inactive() const override {
        return inactive_;
    }

protected:
    std::string logStringImpl (char const* name) const
    {
        std::ostringstream ostr;
        ostr <<
            name << ": " <<
            "\ninIss: " << book_.in.account <<
            "\noutIss: " << book_.out.account <<
            "\ninCur: " << book_.in.currency <<
            "\noutCur: " << book_.out.currency;
        return ostr.str ();
    }

private:
    friend bool operator==(BookStep const& lhs, BookStep const& rhs)
    {
        return lhs.book_ == rhs.book_;
    }

    friend bool operator!=(BookStep const& lhs, BookStep const& rhs)
    {
        return ! (lhs == rhs);
    }

    bool equal (Step const& rhs) const override;

    template <class Callback>
    std::pair<boost::container::flat_set<uint256>, std::uint32_t>
    forEachOffer (
        PaymentSandbox& sb,
        ApplyView& afView,
        bool prevStepRedeems,
        Callback& callback) const;

    void consumeOffer (PaymentSandbox& sb,
        TOffer<TIn, TOut>& offer,
        TAmounts<TIn, TOut> const& ofrAmt,
        TAmounts<TIn, TOut> const& stepAmt,
        TOut const& ownerGives) const;
};



template<class TIn, class TOut>
class BookPaymentStep
    : public BookStep<TIn, TOut, BookPaymentStep<TIn, TOut>>
{
public:
    explicit BookPaymentStep() = default;

    using BookStep<TIn, TOut, BookPaymentStep<TIn, TOut>>::BookStep;
    using BookStep<TIn, TOut, BookPaymentStep<TIn, TOut>>::qualityUpperBound;

    bool limitSelfCrossQuality (AccountID const&, AccountID const&,
        TOffer<TIn, TOut> const& offer, boost::optional<Quality>&,
        FlowOfferStream<TIn, TOut>&, bool) const
    {
        return false;
    }

    bool checkQualityThreshold(TOffer<TIn, TOut> const& offer) const
    {
        return true;
    }

    std::uint32_t getOfrInRate (
        Step const*, TOffer<TIn, TOut> const&, std::uint32_t trIn) const
    {
        return trIn;
    }

    std::uint32_t getOfrOutRate (Step const*, TOffer<TIn, TOut> const&,
        AccountID const&, std::uint32_t trOut) const
    {
        return trOut;
    }

    Quality
    qualityUpperBound(ReadView const& v,
        Quality const& ofrQ,
        bool prevStepRedeems) const
    {
        auto rate = [&](AccountID const& id) {
            if (isXRP(id) || id == this->strandDst_)
                return parityRate;
            return transferRate(v, id);
        };

        auto const trIn =
            prevStepRedeems ? rate(this->book_.in.account) : parityRate;
        auto const trOut =
            this->ownerPaysTransferFee_
            ? rate(this->book_.out.account)
            : parityRate;

        Quality const q1{getRate(STAmount(trOut.value), STAmount(trIn.value))};
        return composed_quality(q1, ofrQ);
    }

    std::string logString () const override
    {
        return this->logStringImpl ("BookPaymentStep");
    }
};

template<class TIn, class TOut>
class BookOfferCrossingStep
    : public BookStep<TIn, TOut, BookOfferCrossingStep<TIn, TOut>>
{
    using BookStep<TIn, TOut, BookOfferCrossingStep<TIn, TOut>>::qualityUpperBound;
private:
    static Quality getQuality (boost::optional<Quality> const& limitQuality)
    {
        assert (limitQuality);
        if (!limitQuality)
            Throw<FlowException> (tefINTERNAL, "Offer requires quality.");
        return *limitQuality;
    }

public:
    BookOfferCrossingStep (
        StrandContext const& ctx, Issue const& in, Issue const& out)
    : BookStep<TIn, TOut, BookOfferCrossingStep<TIn, TOut>> (ctx, in, out)
    , defaultPath_ (ctx.isDefaultPath)
    , qualityThreshold_ (getQuality (ctx.limitQuality))
    {
    }

    bool limitSelfCrossQuality (AccountID const& strandSrc,
        AccountID const& strandDst, TOffer<TIn, TOut> const& offer,
        boost::optional<Quality>& ofrQ, FlowOfferStream<TIn, TOut>& offers,
        bool const offerAttempted) const
    {
        if (defaultPath_ && offer.quality() >= qualityThreshold_ &&
            strandSrc == offer.owner() && strandDst == offer.owner())
        {
            offers.permRmOffer (offer.key());

            if (!offerAttempted)
                ofrQ = boost::none;

            return true;
        }
        return false;
    }

    bool checkQualityThreshold(TOffer<TIn, TOut> const& offer) const
    {
        return !defaultPath_ || offer.quality() >= qualityThreshold_;
    }

    std::uint32_t getOfrInRate (Step const* prevStep,
        TOffer<TIn, TOut> const& offer, std::uint32_t trIn) const
    {
        auto const srcAcct = prevStep ?
            prevStep->directStepSrcAcct() :
            boost::none;

        return                                        
            srcAcct &&                                
            offer.owner() == *srcAcct                 
            ? QUALITY_ONE : trIn;                     
    }

    std::uint32_t getOfrOutRate (
        Step const* prevStep, TOffer<TIn, TOut> const& offer,
        AccountID const& strandDst, std::uint32_t trOut) const
    {
        return                                        
            prevStep && prevStep->bookStepBook() &&   
            offer.owner() == strandDst                
            ? QUALITY_ONE : trOut;                    
    }

    Quality
    qualityUpperBound(ReadView const& v,
        Quality const& ofrQ,
        bool prevStepRedeems) const
    {
        return ofrQ;
    }

    std::string logString () const override
    {
        return this->logStringImpl ("BookOfferCrossingStep");
    }

private:
    bool const defaultPath_;
    Quality const qualityThreshold_;
};


template <class TIn, class TOut, class TDerived>
bool BookStep<TIn, TOut, TDerived>::equal (Step const& rhs) const
{
    if (auto bs = dynamic_cast<BookStep<TIn, TOut, TDerived> const*>(&rhs))
        return book_ == bs->book_;
    return false;
}

template <class TIn, class TOut, class TDerived>
boost::optional<Quality>
BookStep<TIn, TOut, TDerived>::qualityUpperBound(
    ReadView const& v, bool& redeems) const
{
    auto const prevStepRedeems = redeems;
    redeems = this->redeems(v, true);

    Sandbox sb(&v, tapNONE);
    BookTip bt(sb, book_);
    if (!bt.step(j_))
        return boost::none;

    return static_cast<TDerived const*>(this)->qualityUpperBound(
        v, bt.quality(), prevStepRedeems);
}

template <class TIn, class TOut>
static
void limitStepIn (Quality const& ofrQ,
    TAmounts<TIn, TOut>& ofrAmt,
    TAmounts<TIn, TOut>& stpAmt,
    TOut& ownerGives,
    std::uint32_t transferRateIn,
    std::uint32_t transferRateOut,
    TIn const& limit)
{
    if (limit < stpAmt.in)
    {
        stpAmt.in = limit;
        auto const inLmt = mulRatio (
            stpAmt.in, QUALITY_ONE, transferRateIn,  false);
        ofrAmt = ofrQ.ceil_in (ofrAmt, inLmt);
        stpAmt.out = ofrAmt.out;
        ownerGives = mulRatio (
            ofrAmt.out, transferRateOut, QUALITY_ONE,  false);
    }
}

template <class TIn, class TOut>
static
void limitStepOut (Quality const& ofrQ,
    TAmounts<TIn, TOut>& ofrAmt,
    TAmounts<TIn, TOut>& stpAmt,
    TOut& ownerGives,
    std::uint32_t transferRateIn,
    std::uint32_t transferRateOut,
    TOut const& limit)
{
    if (limit < stpAmt.out)
    {
        stpAmt.out = limit;
        ownerGives = mulRatio (
            stpAmt.out, transferRateOut, QUALITY_ONE,  false);
        ofrAmt = ofrQ.ceil_out (ofrAmt, stpAmt.out);
        stpAmt.in = mulRatio (
            ofrAmt.in, transferRateIn, QUALITY_ONE,  true);
    }
}

template <class TIn, class TOut, class TDerived>
template <class Callback>
std::pair<boost::container::flat_set<uint256>, std::uint32_t>
BookStep<TIn, TOut, TDerived>::forEachOffer (
    PaymentSandbox& sb,
    ApplyView& afView,
    bool prevStepRedeems,
    Callback& callback) const
{
    auto rate = [this, &sb](AccountID const& id)->std::uint32_t
    {
        if (isXRP (id) || id == this->strandDst_)
            return QUALITY_ONE;
        return transferRate (sb, id).value;
    };

    std::uint32_t const trIn = prevStepRedeems
        ? rate (book_.in.account)
        : QUALITY_ONE;
    std::uint32_t const trOut = ownerPaysTransferFee_
        ? rate (book_.out.account)
        : QUALITY_ONE;

    typename FlowOfferStream<TIn, TOut>::StepCounter
        counter (maxOffersToConsume_, j_);

    FlowOfferStream<TIn, TOut> offers (
        sb, afView, book_, sb.parentCloseTime (), counter, j_);

    bool const flowCross = afView.rules().enabled(featureFlowCross);
    bool offerAttempted = false;
    boost::optional<Quality> ofrQ;
    while (offers.step ())
    {
        auto& offer = offers.tip ();

        if (!ofrQ)
            ofrQ = offer.quality ();
        else if (*ofrQ != offer.quality ())
            break;

        if (static_cast<TDerived const*>(this)->limitSelfCrossQuality (
            strandSrc_, strandDst_, offer, ofrQ, offers, offerAttempted))
                continue;

        if (flowCross &&
            (!isXRP (offer.issueIn().currency)) &&
            (offer.owner() != offer.issueIn().account))
        {
            auto const& issuerID = offer.issueIn().account;
            auto const issuer = afView.read (keylet::account (issuerID));
            if (issuer && ((*issuer)[sfFlags] & lsfRequireAuth))
            {
                auto const& ownerID = offer.owner();
                auto const authFlag =
                    issuerID > ownerID ? lsfHighAuth : lsfLowAuth;

                auto const line = afView.read (keylet::line (
                    ownerID, issuerID, offer.issueIn().currency));

                if (!line || (((*line)[sfFlags] & authFlag) == 0))
                {
                    offers.permRmOffer (offer.key());
                    if (!offerAttempted)
                        ofrQ = boost::none;
                    continue;
                }
            }
        }

        if (! static_cast<TDerived const*>(this)->checkQualityThreshold(offer))
            break;

        auto const ofrInRate =
            static_cast<TDerived const*>(this)->getOfrInRate (
                prevStep_, offer, trIn);

        auto const ofrOutRate =
            static_cast<TDerived const*>(this)->getOfrOutRate (
                prevStep_, offer, strandDst_, trOut);

        auto ofrAmt = offer.amount ();
        auto stpAmt = make_Amounts (
            mulRatio (ofrAmt.in, ofrInRate, QUALITY_ONE,  true),
            ofrAmt.out);

        auto ownerGives =
            mulRatio (ofrAmt.out, ofrOutRate, QUALITY_ONE,  false);

        auto const funds =
            (offer.owner () == offer.issueOut ().account)
            ? ownerGives 
            : offers.ownerFunds ();

        if (funds < ownerGives)
        {
            ownerGives = funds;
            stpAmt.out = mulRatio (
                ownerGives, QUALITY_ONE, ofrOutRate,  false);
            ofrAmt = ofrQ->ceil_out (ofrAmt, stpAmt.out);
            stpAmt.in = mulRatio (
                ofrAmt.in, ofrInRate, QUALITY_ONE,  true);
        }

        offerAttempted = true;
        if (!callback (
            offer, ofrAmt, stpAmt, ownerGives, ofrInRate, ofrOutRate))
            break;
    }

    return {offers.permToRemove (), counter.count()};
}

template <class TIn, class TOut, class TDerived>
void BookStep<TIn, TOut, TDerived>::consumeOffer (
    PaymentSandbox& sb,
    TOffer<TIn, TOut>& offer,
    TAmounts<TIn, TOut> const& ofrAmt,
    TAmounts<TIn, TOut> const& stepAmt,
    TOut const& ownerGives) const
{
    {
        auto const dr = accountSend (sb, book_.in.account, offer.owner (),
            toSTAmount (ofrAmt.in, book_.in), j_);
        if (dr != tesSUCCESS)
            Throw<FlowException> (dr);
    }

    {
        auto const cr = accountSend (sb, offer.owner (), book_.out.account,
            toSTAmount (ownerGives, book_.out), j_);
        if (cr != tesSUCCESS)
            Throw<FlowException> (cr);
    }

    offer.consume (sb, ofrAmt);
}

template<class TCollection>
static
auto sum (TCollection const& col)
{
    using TResult = std::decay_t<decltype (*col.begin ())>;
    if (col.empty ())
        return TResult{beast::zero};
    return std::accumulate (col.begin () + 1, col.end (), *col.begin ());
};

template<class TIn, class TOut, class TDerived>
std::pair<TIn, TOut>
BookStep<TIn, TOut, TDerived>::revImp (
    PaymentSandbox& sb,
    ApplyView& afView,
    boost::container::flat_set<uint256>& ofrsToRm,
    TOut const& out)
{
    cache_.reset ();

    TAmounts<TIn, TOut> result (beast::zero, beast::zero);

    auto remainingOut = out;

    boost::container::flat_multiset<TIn> savedIns;
    savedIns.reserve(64);
    boost::container::flat_multiset<TOut> savedOuts;
    savedOuts.reserve(64);

    
    auto eachOffer =
        [&](TOffer<TIn, TOut>& offer,
            TAmounts<TIn, TOut> const& ofrAmt,
            TAmounts<TIn, TOut> const& stpAmt,
            TOut const& ownerGives,
            std::uint32_t transferRateIn,
            std::uint32_t transferRateOut) mutable -> bool
    {
        if (remainingOut <= beast::zero)
            return false;

        if (stpAmt.out <= remainingOut)
        {
            savedIns.insert(stpAmt.in);
            savedOuts.insert(stpAmt.out);
            result = TAmounts<TIn, TOut>(sum (savedIns), sum(savedOuts));
            remainingOut = out - result.out;
            this->consumeOffer (sb, offer, ofrAmt, stpAmt, ownerGives);
            return true;
        }
        else
        {
            auto ofrAdjAmt = ofrAmt;
            auto stpAdjAmt = stpAmt;
            auto ownerGivesAdj = ownerGives;
            limitStepOut (offer.quality (), ofrAdjAmt, stpAdjAmt, ownerGivesAdj,
                transferRateIn, transferRateOut, remainingOut);
            remainingOut = beast::zero;
            savedIns.insert (stpAdjAmt.in);
            savedOuts.insert (remainingOut);
            result.in = sum(savedIns);
            result.out = out;
            this->consumeOffer (sb, offer, ofrAdjAmt, stpAdjAmt, ownerGivesAdj);

            if (fix1298(sb.parentCloseTime()))
                return offer.fully_consumed();
            else
                return false;
        }
    };

    {
        auto const prevStepRedeems = prevStep_ && prevStep_->redeems (sb, false);
        auto const r = forEachOffer (sb, afView, prevStepRedeems, eachOffer);
        boost::container::flat_set<uint256> toRm = std::move(std::get<0>(r));
        std::uint32_t const offersConsumed = std::get<1>(r);
        SetUnion(ofrsToRm, toRm);

        if (offersConsumed >= maxOffersToConsume_)
        {
            if (!afView.rules().enabled(fix1515))
            {
                cache_.emplace(beast::zero, beast::zero);
                return {beast::zero, beast::zero};
            }

            inactive_ = true;
        }
    }

    switch(remainingOut.signum())
    {
        case -1:
        {
            JLOG (j_.error ())
                << "BookStep remainingOut < 0 " << to_string (remainingOut);
            assert (0);
            cache_.emplace (beast::zero, beast::zero);
            return {beast::zero, beast::zero};
        }
        case 0:
        {
            result.out = out;
        }
    }

    cache_.emplace (result.in, result.out);
    return {result.in, result.out};
}

template<class TIn, class TOut, class TDerived>
std::pair<TIn, TOut>
BookStep<TIn, TOut, TDerived>::fwdImp (
    PaymentSandbox& sb,
    ApplyView& afView,
    boost::container::flat_set<uint256>& ofrsToRm,
    TIn const& in)
{
    assert(cache_);

    TAmounts<TIn, TOut> result (beast::zero, beast::zero);

    auto remainingIn = in;

    boost::container::flat_multiset<TIn> savedIns;
    savedIns.reserve(64);
    boost::container::flat_multiset<TOut> savedOuts;
    savedOuts.reserve(64);

    auto eachOffer =
        [&](TOffer<TIn, TOut>& offer,
            TAmounts<TIn, TOut> const& ofrAmt,
            TAmounts<TIn, TOut> const& stpAmt,
            TOut const& ownerGives,
            std::uint32_t transferRateIn,
            std::uint32_t transferRateOut) mutable -> bool
    {
        assert(cache_);

        if (remainingIn <= beast::zero)
            return false;

        bool processMore = true;
        auto ofrAdjAmt = ofrAmt;
        auto stpAdjAmt = stpAmt;
        auto ownerGivesAdj = ownerGives;

        typename boost::container::flat_multiset<TOut>::const_iterator lastOut;
        if (stpAmt.in <= remainingIn)
        {
            savedIns.insert(stpAmt.in);
            lastOut = savedOuts.insert(stpAmt.out);
            result = TAmounts<TIn, TOut>(sum (savedIns), sum(savedOuts));
            processMore = true;
        }
        else
        {
            limitStepIn (offer.quality (), ofrAdjAmt, stpAdjAmt, ownerGivesAdj,
                transferRateIn, transferRateOut, remainingIn);
            savedIns.insert (remainingIn);
            lastOut = savedOuts.insert (stpAdjAmt.out);
            result.out = sum (savedOuts);
            result.in = in;

            processMore = false;
        }

        if (result.out > cache_->out && result.in <= cache_->in)
        {
            auto const lastOutAmt = *lastOut;
            savedOuts.erase(lastOut);
            auto const remainingOut = cache_->out - sum (savedOuts);
            auto ofrAdjAmtRev = ofrAmt;
            auto stpAdjAmtRev = stpAmt;
            auto ownerGivesAdjRev = ownerGives;
            limitStepOut (offer.quality (), ofrAdjAmtRev, stpAdjAmtRev,
                ownerGivesAdjRev, transferRateIn, transferRateOut,
                remainingOut);

            if (stpAdjAmtRev.in == remainingIn)
            {
                result.in = in;
                result.out = cache_->out;

                savedIns.clear();
                savedIns.insert(result.in);
                savedOuts.clear();
                savedOuts.insert(result.out);

                ofrAdjAmt = ofrAdjAmtRev;
                stpAdjAmt.in = remainingIn;
                stpAdjAmt.out = remainingOut;
                ownerGivesAdj = ownerGivesAdjRev;
            }
            else
            {
                savedOuts.insert (lastOutAmt);
            }
        }

        remainingIn = in - result.in;
        this->consumeOffer (sb, offer, ofrAdjAmt, stpAdjAmt, ownerGivesAdj);

        if (fix1298(sb.parentCloseTime()))
            processMore = processMore || offer.fully_consumed();

        return processMore;
    };

    {
        auto const prevStepRedeems = prevStep_ && prevStep_->redeems (sb, true);
        auto const r = forEachOffer (sb, afView, prevStepRedeems, eachOffer);
        boost::container::flat_set<uint256> toRm = std::move(std::get<0>(r));
        std::uint32_t const offersConsumed = std::get<1>(r);
        SetUnion(ofrsToRm, toRm);

        if (offersConsumed >= maxOffersToConsume_)
        {
            if (!afView.rules().enabled(fix1515))
            {
                cache_.emplace(beast::zero, beast::zero);
                return {beast::zero, beast::zero};
            }

            inactive_ = true;
        }
    }

    switch(remainingIn.signum())
    {
        case -1:
        {
            JLOG (j_.error ())
                << "BookStep remainingIn < 0 " << to_string (remainingIn);
            assert (0);
            cache_.emplace (beast::zero, beast::zero);
            return {beast::zero, beast::zero};
        }
        case 0:
        {
            result.in = in;
        }
    }

    cache_.emplace (result.in, result.out);
    return {result.in, result.out};
}

template<class TIn, class TOut, class TDerived>
std::pair<bool, EitherAmount>
BookStep<TIn, TOut, TDerived>::validFwd (
    PaymentSandbox& sb,
    ApplyView& afView,
    EitherAmount const& in)
{
    if (!cache_)
    {
        JLOG (j_.trace()) << "Expected valid cache in validFwd";
        return {false, EitherAmount (TOut (beast::zero))};
    }

    auto const savCache = *cache_;

    try
    {
        boost::container::flat_set<uint256> dummy;
        fwdImp (sb, afView, dummy, get<TIn> (in));  
    }
    catch (FlowException const&)
    {
        return {false, EitherAmount (TOut (beast::zero))};
    }

    if (!(checkNear (savCache.in, cache_->in) &&
            checkNear (savCache.out, cache_->out)))
    {
        JLOG (j_.error()) <<
            "Strand re-execute check failed." <<
            " ExpectedIn: " << to_string (savCache.in) <<
            " CachedIn: " << to_string (cache_->in) <<
            " ExpectedOut: " << to_string (savCache.out) <<
            " CachedOut: " << to_string (cache_->out);
        return {false, EitherAmount (cache_->out)};
    }
    return {true, EitherAmount (cache_->out)};
}

template<class TIn, class TOut, class TDerived>
TER
BookStep<TIn, TOut, TDerived>::check(StrandContext const& ctx) const
{
    if (book_.in == book_.out)
    {
        JLOG (j_.debug()) << "BookStep: Book with same in and out issuer " << *this;
        return temBAD_PATH;
    }
    if (!isConsistent (book_.in) || !isConsistent (book_.out))
    {
        JLOG (j_.debug()) << "Book: currency is inconsistent with issuer." << *this;
        return temBAD_PATH;
    }

    if (!ctx.seenBookOuts.insert (book_.out).second ||
        ctx.seenDirectIssues[0].count (book_.out))
    {
        JLOG (j_.debug()) << "BookStep: loop detected: " << *this;
        return temBAD_PATH_LOOP;
    }

    if (ctx.view.rules().enabled(fix1373) &&
        ctx.seenDirectIssues[1].count(book_.out))
    {
        JLOG(j_.debug()) << "BookStep: loop detected: " << *this;
        return temBAD_PATH_LOOP;
    }

    if (fix1443(ctx.view.info().parentCloseTime))
    {
        if (ctx.prevStep)
        {
            if (auto const prev = ctx.prevStep->directStepSrcAcct())
            {
                auto const& view = ctx.view;
                auto const& cur = book_.in.account;

                auto sle =
                    view.read(keylet::line(*prev, cur, book_.in.currency));
                if (!sle)
                    return terNO_LINE;
                if ((*sle)[sfFlags] &
                    ((cur > *prev) ? lsfHighNoRipple : lsfLowNoRipple))
                    return terNO_RIPPLE;
            }
        }
    }

    return tesSUCCESS;
}


namespace test
{

template <class TIn, class TOut, class TDerived>
static
bool equalHelper (Step const& step, ripple::Book const& book)
{
    if (auto bs = dynamic_cast<BookStep<TIn, TOut, TDerived> const*> (&step))
        return book == bs->book ();
    return false;
}

bool bookStepEqual (Step const& step, ripple::Book const& book)
{
    bool const inXRP = isXRP (book.in.currency);
    bool const outXRP = isXRP (book.out.currency);
    if (inXRP && outXRP)
        return equalHelper<XRPAmount, XRPAmount,
            BookPaymentStep<XRPAmount, XRPAmount>> (step, book);
    if (inXRP && !outXRP)
        return equalHelper<XRPAmount, IOUAmount,
            BookPaymentStep<XRPAmount, IOUAmount>> (step, book);
    if (!inXRP && outXRP)
        return equalHelper<IOUAmount, XRPAmount,
            BookPaymentStep<IOUAmount, XRPAmount>> (step, book);
    if (!inXRP && !outXRP)
        return equalHelper<IOUAmount, IOUAmount,
            BookPaymentStep<IOUAmount, IOUAmount>> (step, book);
    return false;
}
}


template <class TIn, class TOut>
static
std::pair<TER, std::unique_ptr<Step>>
make_BookStepHelper (
    StrandContext const& ctx,
    Issue const& in,
    Issue const& out)
{
    TER ter = tefINTERNAL;
    std::unique_ptr<Step> r;
    if (ctx.offerCrossing)
    {
        auto offerCrossingStep =
            std::make_unique<BookOfferCrossingStep<TIn, TOut>> (ctx, in, out);
        ter = offerCrossingStep->check (ctx);
        r = std::move (offerCrossingStep);
    }
    else 
    {
        auto paymentStep =
            std::make_unique<BookPaymentStep<TIn, TOut>> (ctx, in, out);
        ter = paymentStep->check (ctx);
        r = std::move (paymentStep);
    }
    if (ter != tesSUCCESS)
        return {ter, nullptr};

    return {tesSUCCESS, std::move(r)};
}

std::pair<TER, std::unique_ptr<Step>>
make_BookStepII (
    StrandContext const& ctx,
    Issue const& in,
    Issue const& out)
{
    return make_BookStepHelper<IOUAmount, IOUAmount> (ctx, in, out);
}

std::pair<TER, std::unique_ptr<Step>>
make_BookStepIX (
    StrandContext const& ctx,
    Issue const& in)
{
    return make_BookStepHelper<IOUAmount, XRPAmount> (ctx, in, xrpIssue());
}

std::pair<TER, std::unique_ptr<Step>>
make_BookStepXI (
    StrandContext const& ctx,
    Issue const& out)
{
    return make_BookStepHelper<XRPAmount, IOUAmount> (ctx, xrpIssue(), out);
}

} 
























