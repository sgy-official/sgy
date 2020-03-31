

#ifndef RIPPLE_APP_TX_TRANSACTOR_H_INCLUDED
#define RIPPLE_APP_TX_TRANSACTOR_H_INCLUDED

#include <ripple/app/tx/applySteps.h>
#include <ripple/app/tx/impl/ApplyContext.h>
#include <ripple/protocol/XRPAmount.h>
#include <ripple/beast/utility/Journal.h>
#include <boost/optional.hpp>

namespace ripple {


struct PreflightContext
{
public:
    Application& app;
    STTx const& tx;
    Rules const rules;
    ApplyFlags flags;
    beast::Journal j;

    PreflightContext(Application& app_, STTx const& tx_,
        Rules const& rules_, ApplyFlags flags_,
            beast::Journal j_);

    PreflightContext& operator=(PreflightContext const&) = delete;
};


struct PreclaimContext
{
public:
    Application& app;
    ReadView const& view;
    TER preflightResult;
    STTx const& tx;
    ApplyFlags flags;
    beast::Journal j;

    PreclaimContext(Application& app_, ReadView const& view_,
        TER preflightResult_, STTx const& tx_,
            ApplyFlags flags_,
            beast::Journal j_ = beast::Journal{beast::Journal::getNullSink()})
        : app(app_)
        , view(view_)
        , preflightResult(preflightResult_)
        , tx(tx_)
        , flags(flags_)
        , j(j_)
    {
    }

    PreclaimContext& operator=(PreclaimContext const&) = delete;
};

struct TxConsequences;
struct PreflightResult;

class Transactor
{
protected:
    ApplyContext& ctx_;
    beast::Journal j_;

    AccountID     account_;
    XRPAmount     mPriorBalance;  
    XRPAmount     mSourceBalance; 

    virtual ~Transactor() = default;
    Transactor (Transactor const&) = delete;
    Transactor& operator= (Transactor const&) = delete;

public:
    
    std::pair<TER, bool>
    operator()();

    ApplyView&
    view()
    {
        return ctx_.view();
    }

    ApplyView const&
    view() const
    {
        return ctx_.view();
    }

    

    static
    NotTEC
    checkSeq (PreclaimContext const& ctx);

    static
    TER
    checkFee (PreclaimContext const& ctx, std::uint64_t baseFee);

    static
    NotTEC
    checkSign (PreclaimContext const& ctx);

    static
    std::uint64_t
    calculateBaseFee (
        ReadView const& view,
        STTx const& tx);

    static
    bool
    affectsSubsequentTransactionAuth(STTx const& tx)
    {
        return false;
    }

    static
    XRPAmount
    calculateFeePaid(STTx const& tx);

    static
    XRPAmount
    calculateMaxSpend(STTx const& tx);

    static
    TER
    preclaim(PreclaimContext const &ctx)
    {
        return tesSUCCESS;
    }

protected:
    TER
    apply();

    explicit
    Transactor (ApplyContext& ctx);

    virtual void preCompute();

    virtual TER doApply () = 0;

    
    static
    XRPAmount
    minimumFee (Application& app, std::uint64_t baseFee,
        Fees const& fees, ApplyFlags flags);

private:
    XRPAmount reset(XRPAmount fee);

    void setSeq ();
    TER payFee ();
    static NotTEC checkSingleSign (PreclaimContext const& ctx);
    static NotTEC checkMultiSign (PreclaimContext const& ctx);
};


NotTEC
preflight0(PreflightContext const& ctx);


NotTEC
preflight1 (PreflightContext const& ctx);


NotTEC
preflight2 (PreflightContext const& ctx);

}

#endif








