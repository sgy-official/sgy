

#ifndef BEAST_INSIGHT_HOOKIMPL_H_INCLUDED
#define BEAST_INSIGHT_HOOKIMPL_H_INCLUDED

#include <ripple/beast/insight/BaseImpl.h>

namespace beast {
namespace insight {

class HookImpl
    : public std::enable_shared_from_this <HookImpl>
    , public BaseImpl
{
public:
    using HandlerType = std::function <void (void)>;

    virtual ~HookImpl () = 0;
};

}
}

#endif








