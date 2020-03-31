

#include <ripple/ledger/ApplyViewImpl.h>
#include <ripple/basics/contract.h>
#include <cassert>

namespace ripple {

ApplyViewImpl::ApplyViewImpl(
    ReadView const* base, ApplyFlags flags)
    : ApplyViewBase (base, flags)
{
}

void
ApplyViewImpl::apply (OpenView& to,
    STTx const& tx, TER ter,
        beast::Journal j)
{
    items_.apply(to, tx, ter, deliver_, j);
}

std::size_t
ApplyViewImpl::size ()
{
    return items_.size ();
}

void
ApplyViewImpl::visit (
    OpenView& to,
    std::function <void (
        uint256 const& key,
        bool isDelete,
        std::shared_ptr <SLE const> const& before,
        std::shared_ptr <SLE const> const& after)> const& func)
{
    items_.visit (to, func);
}

} 
























