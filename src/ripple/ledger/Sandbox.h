

#ifndef RIPPLE_LEDGER_SANDBOX_H_INCLUDED
#define RIPPLE_LEDGER_SANDBOX_H_INCLUDED

#include <ripple/ledger/RawView.h>
#include <ripple/ledger/detail/ApplyViewBase.h>

namespace ripple {


class Sandbox
    : public detail::ApplyViewBase
{
public:
    Sandbox() = delete;
    Sandbox (Sandbox const&) = delete;
    Sandbox& operator= (Sandbox&&) = delete;
    Sandbox& operator= (Sandbox const&) = delete;

    Sandbox (Sandbox&&) = default;

    Sandbox (ReadView const* base, ApplyFlags flags)
        : ApplyViewBase (base, flags)
    {
    }

    Sandbox (ApplyView const* base)
        : Sandbox(base, base->flags())
    {
    }

    void
    apply (RawView& to)
    {
        items_.apply(to);
    }
};

} 

#endif








