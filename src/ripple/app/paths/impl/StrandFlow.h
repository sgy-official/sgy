

#ifndef RIPPLE_APP_PATHS_IMPL_STRANDFLOW_H_INCLUDED
#define RIPPLE_APP_PATHS_IMPL_STRANDFLOW_H_INCLUDED

#include <ripple/app/paths/Credit.h>
#include <ripple/app/paths/Flow.h>
#include <ripple/app/paths/impl/AmountSpec.h>
#include <ripple/app/paths/impl/FlatSets.h>
#include <ripple/app/paths/impl/FlowDebugInfo.h>
#include <ripple/app/paths/impl/Steps.h>
#include <ripple/basics/Log.h>
#include <ripple/protocol/IOUAmount.h>
#include <ripple/protocol/XRPAmount.h>

#include <boost/container/flat_set.hpp>

#include <algorithm>
#include <iterator>
#include <numeric>
#include <sstream>

namespace ripple {


template<class TInAmt, class TOutAmt>
struct StrandResult
{
    TER ter = temUNKNOWN;                         
    TInAmt in = beast::zero;                      
    TOutAmt out = beast::zero;                    
    boost::optional<PaymentSandbox> sandbox;      
    boost::container::flat_set<uint256> ofrsToRm; 
    bool inactive = false; 

    
    StrandResult () = default;

    StrandResult (TInAmt const& in_,
        TOutAmt const& out_,
        PaymentSandbox&& sandbox_,
        boost::container::flat_set<uint256> ofrsToRm_,
        bool inactive_)
        : ter (tesSUCCESS)
        , in (in_)
        , out (out_)
        , sandbox (std::move (sandbox_))
        , ofrsToRm (std::move (ofrsToRm_))
        , inactive(inactive_)
    {
    }

    StrandResult (TER ter_, boost::container::flat_set<uint256> ofrsToRm_)
        : ter (ter_), ofrsToRm (std::move (ofrsToRm_))
    {
    }
};


template<class TInAmt, class TOutAmt>
StrandResult <TInAmt, TOutAmt>
flow (
    PaymentSandbox const& baseView,
    Strand const& strand,
    boost::optional<TInAmt> const& maxIn,
    TOutAmt const& out,
    beast::Journal j)
{
    using Result = StrandResult<TInAmt, TOutAmt>;
    if (strand.empty ())
    {
        JLOG (j.warn()) << "Empty strand passed to Liquidity";
        return {};
    }

    boost::container::flat_set<uint256> ofrsToRm;

    if (isDirectXrpToXrp<TInAmt, TOutAmt> (strand))
    {
        return {tecNO_LINE, std::move (ofrsToRm)};
    }

    try
    {
        std::size_t const s = strand.size ();

        std::size_t limitingStep = strand.size ();
        boost::optional<PaymentSandbox> sb (&baseView);
        boost::optional<PaymentSandbox> afView (&baseView);
        EitherAmount limitStepOut;
        {
            EitherAmount stepOut (out);
            for (auto i = s; i--;)
            {
                auto r = strand[i]->rev (*sb, *afView, ofrsToRm, stepOut);
                if (strand[i]->isZero (r.second))
                {
                    JLOG (j.trace()) << "Strand found dry in rev";
                    return {tecPATH_DRY, std::move (ofrsToRm)};
                }

                if (i == 0 && maxIn && *maxIn < get<TInAmt> (r.first))
                {
                    sb.emplace (&baseView);
                    limitingStep = i;

                    r = strand[i]->fwd (
                        *sb, *afView, ofrsToRm, EitherAmount (*maxIn));
                    limitStepOut = r.second;

                    if (strand[i]->isZero(r.second))
                    {
                        JLOG(j.trace()) << "First step found dry";
                        return {tecPATH_DRY, std::move(ofrsToRm)};
                    }
                    if (get<TInAmt> (r.first) != *maxIn)
                    {
                        JLOG(j.fatal())
                            << "Re-executed limiting step failed. r.first: "
                            << to_string(get<TInAmt>(r.first))
                            << " maxIn: " << to_string(*maxIn);
                        assert(0);
                        return {telFAILED_PROCESSING, std::move (ofrsToRm)};
                    }
                }
                else if (!strand[i]->equalOut (r.second, stepOut))
                {
                    sb.emplace (&baseView);
                    afView.emplace (&baseView);
                    limitingStep = i;

                    stepOut = r.second;
                    r = strand[i]->rev (*sb, *afView, ofrsToRm, stepOut);
                    limitStepOut = r.second;

                    if (strand[i]->isZero(r.second))
                    {
                        JLOG(j.trace()) << "Limiting step found dry";
                        return {tecPATH_DRY, std::move(ofrsToRm)};
                    }
                    if (!strand[i]->equalOut (r.second, stepOut))
                    {
#ifndef NDEBUG
                        JLOG(j.fatal())
                            << "Re-executed limiting step failed. r.second: "
                            << r.second << " stepOut: " << stepOut;
#else
                        JLOG (j.fatal()) << "Re-executed limiting step failed";
#endif
                        assert (0);
                        return {telFAILED_PROCESSING, std::move (ofrsToRm)};
                    }
                }

                stepOut = r.first;
            }
        }

        {
            EitherAmount stepIn (limitStepOut);
            for (auto i = limitingStep + 1; i < s; ++i)
            {
                auto const r = strand[i]->fwd(*sb, *afView, ofrsToRm, stepIn);
                if (strand[i]->isZero(r.second))
                {
                    JLOG(j.trace()) << "Non-limiting step found dry";
                    return {tecPATH_DRY, std::move(ofrsToRm)};
                }
                if (!strand[i]->equalIn (r.first, stepIn))
                {
#ifndef NDEBUG
                    JLOG(j.fatal())
                        << "Re-executed forward pass failed. r.first: "
                        << r.first << " stepIn: " << stepIn;
#else
                    JLOG (j.fatal()) << "Re-executed forward pass failed";
#endif
                    assert (0);
                    return {telFAILED_PROCESSING, std::move (ofrsToRm)};
                }
                stepIn = r.second;
            }
        }

        auto const strandIn = *strand.front ()->cachedIn ();
        auto const strandOut = *strand.back ()->cachedOut ();

#ifndef NDEBUG
        {
            PaymentSandbox checkSB (&baseView);
            PaymentSandbox checkAfView (&baseView);
            EitherAmount stepIn (*strand[0]->cachedIn ());
            for (auto i = 0; i < s; ++i)
            {
                bool valid;
                std::tie (valid, stepIn) =
                    strand[i]->validFwd (checkSB, checkAfView, stepIn);
                if (!valid)
                {
                    JLOG (j.error())
                        << "Strand re-execute check failed. Step: " << i;
                    break;
                }
            }
        }
#endif

        bool const inactive = std::any_of(
            strand.begin(),
            strand.end(),
            [](std::unique_ptr<Step> const& step) { return step->inactive(); });

        return Result(
            get<TInAmt>(strandIn),
            get<TOutAmt>(strandOut),
            std::move(*sb),
            std::move(ofrsToRm),
            inactive);
    }
    catch (FlowException const& e)
    {
        return {e.ter, std::move (ofrsToRm)};
    }
}

template<class TInAmt, class TOutAmt>
struct FlowResult
{
    TInAmt in = beast::zero;
    TOutAmt out = beast::zero;
    boost::optional<PaymentSandbox> sandbox;
    boost::container::flat_set<uint256> removableOffers;
    TER ter = temUNKNOWN;

    FlowResult () = default;

    FlowResult (TInAmt const& in_,
        TOutAmt const& out_,
        PaymentSandbox&& sandbox_,
        boost::container::flat_set<uint256> ofrsToRm)
        : in (in_)
        , out (out_)
        , sandbox (std::move (sandbox_))
        , removableOffers(std::move (ofrsToRm))
        , ter (tesSUCCESS)
    {
    }

    FlowResult (TER ter_, boost::container::flat_set<uint256> ofrsToRm)
        : removableOffers(std::move (ofrsToRm))
        , ter (ter_)
    {
    }

    FlowResult (TER ter_,
        TInAmt const& in_,
        TOutAmt const& out_,
        boost::container::flat_set<uint256> ofrsToRm)
        : in (in_)
        , out (out_)
        , removableOffers (std::move (ofrsToRm))
        , ter (ter_)
    {
    }
};


class ActiveStrands
{
private:
    std::vector<Strand const*> cur_;
    std::vector<Strand const*> next_;

public:
    ActiveStrands (std::vector<Strand> const& strands)
    {
        cur_.reserve (strands.size ());
        next_.reserve (strands.size ());
        for (auto& strand : strands)
            next_.push_back (&strand);
    }

    void
    activateNext ()
    {
        cur_.clear ();
        std::swap (cur_, next_);
    }

    void
    push (Strand const* s)
    {
        next_.push_back (s);
    }

    auto begin ()
    {
        return cur_.begin ();
    }

    auto end ()
    {
        return cur_.end ();
    }

    auto begin () const
    {
        return cur_.begin ();
    }

    auto end () const
    {
        return cur_.end ();
    }

    auto size () const
    {
        return cur_.size ();
    }

    void
    removeIndex(std::size_t i)
    {
        if (i >= next_.size())
            return;
        next_.erase(next_.begin() + i);
    }
};

boost::optional<Quality>
qualityUpperBound(ReadView const& v, Strand const& strand)
{
    Quality q{STAmount::uRateOne};
    bool redeems = false;
    for(auto const& step : strand)
    {
        if (auto const stepQ = step->qualityUpperBound(v, redeems))
            q = composed_quality(q, *stepQ);
        else
            return boost::none;
    }
    return q;
};


template <class TInAmt, class TOutAmt>
FlowResult<TInAmt, TOutAmt>
flow (PaymentSandbox const& baseView,
    std::vector<Strand> const& strands,
    TOutAmt const& outReq,
    bool partialPayment,
    bool offerCrossing,
    boost::optional<Quality> const& limitQuality,
    boost::optional<STAmount> const& sendMaxST,
    beast::Journal j,
    path::detail::FlowDebugInfo* flowDebugInfo=nullptr)
{
    struct BestStrand
    {
        TInAmt in;
        TOutAmt out;
        PaymentSandbox sb;
        Strand const& strand;
        Quality quality;

        BestStrand (TInAmt const& in_,
            TOutAmt const& out_,
            PaymentSandbox&& sb_,
            Strand const& strand_,
            Quality const& quality_)
            : in (in_)
            , out (out_)
            , sb (std::move (sb_))
            , strand (strand_)
            , quality (quality_)
        {
        }
    };

    std::size_t const maxTries = 1000;
    std::size_t curTry = 0;

    TInAmt const sendMaxInit =
        sendMaxST ? toAmount<TInAmt>(*sendMaxST) : TInAmt{beast::zero};
    boost::optional<TInAmt> const sendMax =
        boost::make_optional(sendMaxST && sendMaxInit >= beast::zero, sendMaxInit);
    boost::optional<TInAmt> remainingIn =
        boost::make_optional(!!sendMax, sendMaxInit);

    TOutAmt remainingOut (outReq);

    PaymentSandbox sb (&baseView);

    ActiveStrands activeStrands (strands);

    boost::container::flat_multiset<TInAmt> savedIns;
    savedIns.reserve(maxTries);
    boost::container::flat_multiset<TOutAmt> savedOuts;
    savedOuts.reserve(maxTries);

    auto sum = [](auto const& col)
    {
        using TResult = std::decay_t<decltype (*col.begin ())>;
        if (col.empty ())
            return TResult{beast::zero};
        return std::accumulate (col.begin () + 1, col.end (), *col.begin ());
    };

    boost::container::flat_set<uint256> ofrsToRmOnFail;

    while (remainingOut > beast::zero &&
        (!remainingIn || *remainingIn > beast::zero))
    {
        ++curTry;
        if (curTry >= maxTries)
        {
            return {telFAILED_PROCESSING, std::move(ofrsToRmOnFail)};
        }

        activeStrands.activateNext();

        boost::container::flat_set<uint256> ofrsToRm;
        boost::optional<BestStrand> best;
        if (flowDebugInfo) flowDebugInfo->newLiquidityPass();
        boost::optional<std::size_t> markInactiveOnUse{false, 0};
        for (auto strand : activeStrands)
        {
            if (offerCrossing && limitQuality)
            {
                auto const strandQ = qualityUpperBound(sb, *strand);
                if (!strandQ || *strandQ < *limitQuality)
                    continue;
            }
            auto f = flow<TInAmt, TOutAmt> (
                sb, *strand, remainingIn, remainingOut, j);

            SetUnion(ofrsToRm, f.ofrsToRm);

            if (f.ter != tesSUCCESS || f.out == beast::zero)
                continue;

            if (flowDebugInfo)
                flowDebugInfo->pushLiquiditySrc(EitherAmount(f.in), EitherAmount(f.out));

            assert (f.out <= remainingOut && f.sandbox &&
                (!remainingIn || f.in <= *remainingIn));

            Quality const q (f.out, f.in);

            JLOG (j.trace())
                << "New flow iter (iter, in, out): "
                << curTry-1 << " "
                << to_string(f.in) << " "
                << to_string(f.out);

            if (limitQuality && q < *limitQuality)
            {
                JLOG (j.trace())
                    << "Path rejected by limitQuality"
                    << " limit: " << *limitQuality
                    << " path q: " << q;
                continue;
            }

            activeStrands.push (strand);

            if (!best || best->quality < q ||
                (best->quality == q && best->out < f.out))
            {

                if (f.inactive)
                    markInactiveOnUse = activeStrands.size() - 1;
                else
                    markInactiveOnUse.reset();

                best.emplace(f.in, f.out, std::move(*f.sandbox), *strand, q);
            }
        }

        bool const shouldBreak = !best;

        if (best)
        {
            if (markInactiveOnUse)
            {
                activeStrands.removeIndex(*markInactiveOnUse);
                markInactiveOnUse.reset();
            }
            savedIns.insert (best->in);
            savedOuts.insert (best->out);
            remainingOut = outReq - sum (savedOuts);
            if (sendMax)
                remainingIn = *sendMax - sum (savedIns);

            if (flowDebugInfo)
                flowDebugInfo->pushPass (EitherAmount (best->in),
                    EitherAmount (best->out), activeStrands.size ());

            JLOG (j.trace())
                << "Best path: in: " << to_string (best->in)
                << " out: " << to_string (best->out)
                << " remainingOut: " << to_string (remainingOut);

            best->sb.apply (sb);
        }
        else
        {
            JLOG (j.trace()) << "All strands dry.";
        }

        best.reset ();  
        if (!ofrsToRm.empty ())
        {
            SetUnion(ofrsToRmOnFail, ofrsToRm);
            for (auto const& o : ofrsToRm)
            {
                if (auto ok = sb.peek (keylet::offer (o)))
                    offerDelete (sb, ok, j);
            }
        }

        if (shouldBreak)
            break;
    }

    auto const actualOut = sum (savedOuts);
    auto const actualIn = sum (savedIns);

    JLOG (j.trace())
        << "Total flow: in: " << to_string (actualIn)
        << " out: " << to_string (actualOut);

    if (actualOut != outReq)
    {
        if (actualOut > outReq)
        {
            assert (0);
            return {tefEXCEPTION, std::move(ofrsToRmOnFail)};
        }
        if (!partialPayment)
        {
            if (!offerCrossing)
                return {tecPATH_PARTIAL,
                    actualIn, actualOut, std::move(ofrsToRmOnFail)};
        }
        else if (actualOut == beast::zero)
        {
            return {tecPATH_DRY, std::move(ofrsToRmOnFail)};
        }
    }
    if (offerCrossing && !partialPayment)
    {
        assert (remainingIn);
        if (remainingIn && *remainingIn != beast::zero)
            return {tecPATH_PARTIAL,
                actualIn, actualOut, std::move(ofrsToRmOnFail)};
    }

    return {actualIn, actualOut, std::move (sb), std::move(ofrsToRmOnFail)};
}

} 

#endif








