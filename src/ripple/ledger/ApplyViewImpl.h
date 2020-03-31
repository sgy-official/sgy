

#ifndef RIPPLE_LEDGER_APPLYVIEWIMPL_H_INCLUDED
#define RIPPLE_LEDGER_APPLYVIEWIMPL_H_INCLUDED

#include <ripple/ledger/OpenView.h>
#include <ripple/ledger/detail/ApplyViewBase.h>
#include <ripple/protocol/STAmount.h>
#include <ripple/protocol/TER.h>
#include <boost/optional.hpp>

namespace ripple {


class ApplyViewImpl final
    : public detail::ApplyViewBase
{
public:
    ApplyViewImpl() = delete;
    ApplyViewImpl (ApplyViewImpl const&) = delete;
    ApplyViewImpl& operator= (ApplyViewImpl&&) = delete;
    ApplyViewImpl& operator= (ApplyViewImpl const&) = delete;

    ApplyViewImpl (ApplyViewImpl&&) = default;
    ApplyViewImpl(
        ReadView const* base, ApplyFlags flags);

    
    void
    apply (OpenView& to,
        STTx const& tx, TER ter,
            beast::Journal j);

    
    void
    deliver (STAmount const& amount)
    {
        deliver_ = amount;
    }

    
    std::size_t
    size ();

    
    void
    visit (
        OpenView& target,
        std::function <void (
            uint256 const& key,
            bool isDelete,
            std::shared_ptr <SLE const> const& before,
            std::shared_ptr <SLE const> const& after)> const& func);
private:
    boost::optional<STAmount> deliver_;
};

} 

#endif








