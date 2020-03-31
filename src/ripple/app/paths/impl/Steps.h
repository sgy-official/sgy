

#ifndef RIPPLE_APP_PATHS_IMPL_PAYSTEPS_H_INCLUDED
#define RIPPLE_APP_PATHS_IMPL_PAYSTEPS_H_INCLUDED

#include <ripple/app/paths/impl/AmountSpec.h>
#include <ripple/basics/Log.h>
#include <ripple/protocol/Quality.h>
#include <ripple/protocol/STLedgerEntry.h>
#include <ripple/protocol/TER.h>

#include <boost/container/flat_set.hpp>
#include <boost/optional.hpp>

namespace ripple {
class PaymentSandbox;
class ReadView;
class ApplyView;


class Step
{
public:
    
    virtual ~Step () = default;

    
    virtual
    std::pair<EitherAmount, EitherAmount>
    rev (
        PaymentSandbox& sb,
        ApplyView& afView,
        boost::container::flat_set<uint256>& ofrsToRm,
        EitherAmount const& out) = 0;

    
    virtual
    std::pair<EitherAmount, EitherAmount>
    fwd (
        PaymentSandbox& sb,
        ApplyView& afView,
        boost::container::flat_set<uint256>& ofrsToRm,
        EitherAmount const& in) = 0;

    
    virtual
    boost::optional<EitherAmount>
    cachedIn () const = 0;

    
    virtual
    boost::optional<EitherAmount>
    cachedOut () const = 0;

    
    virtual boost::optional<AccountID>
    directStepSrcAcct () const
    {
        return boost::none;
    }

    virtual boost::optional<std::pair<AccountID,AccountID>>
    directStepAccts () const
    {
        return boost::none;
    }

    
    virtual bool
    redeems (ReadView const& sb, bool fwd) const
    {
        return false;
    }

    
    virtual std::uint32_t
    lineQualityIn (ReadView const&) const
    {
        return QUALITY_ONE;
    }

    
    virtual boost::optional<Quality>
    qualityUpperBound(ReadView const& v, bool& redeems) const = 0;

    
    virtual boost::optional<Book>
    bookStepBook () const
    {
        return boost::none;
    }

    
    virtual
    bool
    isZero (EitherAmount const& out) const = 0;

    
    virtual
    bool inactive() const{
        return false;
    }

    
    virtual
    bool
    equalOut (
        EitherAmount const& lhs,
        EitherAmount const& rhs) const = 0;

    
    virtual bool equalIn (
        EitherAmount const& lhs,
        EitherAmount const& rhs) const = 0;

    
    virtual
    std::pair<bool, EitherAmount>
    validFwd (
        PaymentSandbox& sb,
        ApplyView& afView,
        EitherAmount const& in) = 0;

    
    friend bool operator==(Step const& lhs, Step const& rhs)
    {
        return lhs.equal (rhs);
    }

    
    friend bool operator!=(Step const& lhs, Step const& rhs)
    {
        return ! (lhs == rhs);
    }

    
    friend
    std::ostream&
    operator << (
        std::ostream& stream,
        Step const& step)
    {
        stream << step.logString ();
        return stream;
    }

private:
    virtual
    std::string
    logString () const = 0;

    virtual bool equal (Step const& rhs) const = 0;
};

using Strand = std::vector<std::unique_ptr<Step>>;

inline
bool operator==(Strand const& lhs, Strand const& rhs)
{
    if (lhs.size () != rhs.size ())
        return false;
    for (size_t i = 0, e = lhs.size (); i != e; ++i)
        if (*lhs[i] != *rhs[i])
            return false;
    return true;
}


std::pair<TER, STPath>
normalizePath(AccountID const& src,
    AccountID const& dst,
    Issue const& deliver,
    boost::optional<Issue> const& sendMaxIssue,
    STPath const& path);


std::pair<TER, Strand>
toStrand (
    ReadView const& sb,
    AccountID const& src,
    AccountID const& dst,
    Issue const& deliver,
    boost::optional<Quality> const& limitQuality,
    boost::optional<Issue> const& sendMaxIssue,
    STPath const& path,
    bool ownerPaysTransferFee,
    bool offerCrossing,
    beast::Journal j);


std::pair<TER, std::vector<Strand>>
toStrands (ReadView const& sb,
    AccountID const& src,
    AccountID const& dst,
    Issue const& deliver,
    boost::optional<Quality> const& limitQuality,
    boost::optional<Issue> const& sendMax,
    STPathSet const& paths,
    bool addDefaultPath,
    bool ownerPaysTransferFee,
    bool offerCrossing,
    beast::Journal j);

template <class TIn, class TOut, class TDerived>
struct StepImp : public Step
{
    explicit StepImp() = default;

    std::pair<EitherAmount, EitherAmount>
    rev (
        PaymentSandbox& sb,
        ApplyView& afView,
        boost::container::flat_set<uint256>& ofrsToRm,
        EitherAmount const& out) override
    {
        auto const r =
            static_cast<TDerived*> (this)->revImp (sb, afView, ofrsToRm, get<TOut>(out));
        return {EitherAmount (r.first), EitherAmount (r.second)};
    }

    std::pair<EitherAmount, EitherAmount>
    fwd (
        PaymentSandbox& sb,
        ApplyView& afView,
        boost::container::flat_set<uint256>& ofrsToRm,
        EitherAmount const& in) override
    {
        auto const r =
            static_cast<TDerived*> (this)->fwdImp (sb, afView, ofrsToRm, get<TIn>(in));
        return {EitherAmount (r.first), EitherAmount (r.second)};
    }

    bool
    isZero (EitherAmount const& out) const override
    {
        return get<TOut>(out) == beast::zero;
    }

    bool
    equalOut (EitherAmount const& lhs, EitherAmount const& rhs) const override
    {
        return get<TOut> (lhs) == get<TOut> (rhs);
    }

    bool
    equalIn (EitherAmount const& lhs, EitherAmount const& rhs) const override
    {
        return get<TIn> (lhs) == get<TIn> (rhs);
    }
};

class FlowException : public std::runtime_error
{
public:
    TER ter;
    std::string msg;

    FlowException (TER t, std::string const& msg)
        : std::runtime_error (msg)
        , ter (t)
    {
    }

    explicit
    FlowException (TER t)
        : std::runtime_error (transHuman (t))
        , ter (t)
    {
    }
};

bool checkNear (IOUAmount const& expected, IOUAmount const& actual);
bool checkNear (XRPAmount const& expected, XRPAmount const& actual);


struct StrandContext
{
    ReadView const& view;                        
    AccountID const strandSrc;                   
    AccountID const strandDst;                   
    Issue const strandDeliver;                   
    boost::optional<Quality> const limitQuality; 
    bool const isFirst;                
    bool const isLast = false;         
    bool const ownerPaysTransferFee;   
    bool const offerCrossing;          
    bool const isDefaultPath;          
    size_t const strandSize;           
    
    Step const* const prevStep = nullptr;
    
    std::array<boost::container::flat_set<Issue>, 2>& seenDirectIssues;
    
    boost::container::flat_set<Issue>& seenBookOuts;
    beast::Journal j;                    

    
    StrandContext (ReadView const& view_,
        std::vector<std::unique_ptr<Step>> const& strand_,
        AccountID const& strandSrc_,
        AccountID const& strandDst_,
        Issue const& strandDeliver_,
        boost::optional<Quality> const& limitQuality_,
        bool isLast_,
        bool ownerPaysTransferFee_,
        bool offerCrossing_,
        bool isDefaultPath_,
        std::array<boost::container::flat_set<Issue>, 2>&
            seenDirectIssues_,                             
        boost::container::flat_set<Issue>& seenBookOuts_,  
        beast::Journal j_);                                
};

namespace test {
bool directStepEqual (Step const& step,
    AccountID const& src,
    AccountID const& dst,
    Currency const& currency);

bool xrpEndpointStepEqual (Step const& step, AccountID const& acc);

bool bookStepEqual (Step const& step, ripple::Book const& book);
}

std::pair<TER, std::unique_ptr<Step>>
make_DirectStepI (
    StrandContext const& ctx,
    AccountID const& src,
    AccountID const& dst,
    Currency const& c);

std::pair<TER, std::unique_ptr<Step>>
make_BookStepII (
    StrandContext const& ctx,
    Issue const& in,
    Issue const& out);

std::pair<TER, std::unique_ptr<Step>>
make_BookStepIX (
    StrandContext const& ctx,
    Issue const& in);

std::pair<TER, std::unique_ptr<Step>>
make_BookStepXI (
    StrandContext const& ctx,
    Issue const& out);

std::pair<TER, std::unique_ptr<Step>>
make_XRPEndpointStep (
    StrandContext const& ctx,
    AccountID const& acc);

template<class InAmt, class OutAmt>
bool
isDirectXrpToXrp(Strand const& strand);

} 

#endif








