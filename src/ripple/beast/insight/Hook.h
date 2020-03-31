

#ifndef BEAST_INSIGHT_HOOK_H_INCLUDED
#define BEAST_INSIGHT_HOOK_H_INCLUDED

#include <ripple/beast/insight/Base.h>
#include <ripple/beast/insight/HookImpl.h>

#include <memory>

namespace beast {
namespace insight {


class Hook : public Base
{
public:
    
    Hook ()
        { }

    
    explicit Hook (std::shared_ptr <HookImpl> const& impl)
        : m_impl (impl)
        { }

    std::shared_ptr <HookImpl> const& impl () const
    {
        return m_impl;
    }

private:
    std::shared_ptr <HookImpl> m_impl;
};

}
}

#endif








