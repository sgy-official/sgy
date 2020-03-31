

#ifndef RIPPLE_TX_APPLYCONTEXT_H_INCLUDED
#define RIPPLE_TX_APPLYCONTEXT_H_INCLUDED

#include <ripple/app/main/Application.h>
#include <ripple/ledger/ApplyViewImpl.h>
#include <ripple/core/Config.h>
#include <ripple/protocol/STTx.h>
#include <ripple/protocol/XRPAmount.h>
#include <ripple/beast/utility/Journal.h>
#include <boost/optional.hpp>
#include <utility>

namespace ripple {



class ApplyContext
{
public:
    explicit
    ApplyContext (Application& app, OpenView& base,
        STTx const& tx, TER preclaimResult,
            std::uint64_t baseFee, ApplyFlags flags,
                beast::Journal = beast::Journal{beast::Journal::getNullSink()});

    Application& app;
    STTx const& tx;
    TER const preclaimResult;
    std::uint64_t const baseFee;
    beast::Journal const journal;

    ApplyView&
    view()
    {
        return *view_;
    }

    ApplyView const&
    view() const
    {
        return *view_;
    }

    RawView&
    rawView()
    {
        return *view_;
    }

    
    void
    deliver (STAmount const& amount)
    {
        view_->deliver(amount);
    }

    
    void
    discard();

    
    void
    apply (TER);

    
    std::size_t
    size ();

    
    void
    visit (std::function <void (
        uint256 const& key,
        bool isDelete,
        std::shared_ptr <SLE const> const& before,
        std::shared_ptr <SLE const> const& after)> const& func);

    void
    destroyXRP (XRPAmount const& fee)
    {
        view_->rawDestroyXRP(fee);
    }

    
    TER
    checkInvariants(TER const result, XRPAmount const fee);

private:
    TER
    failInvariantCheck (TER const result);

    template<std::size_t... Is>
    TER
    checkInvariantsHelper(TER const result, XRPAmount const fee, std::index_sequence<Is...>);

    OpenView& base_;
    ApplyFlags flags_;
    boost::optional<ApplyViewImpl> view_;
};

} 

#endif








