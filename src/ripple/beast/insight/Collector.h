

#ifndef BEAST_INSIGHT_COLLECTOR_H_INCLUDED
#define BEAST_INSIGHT_COLLECTOR_H_INCLUDED

#include <ripple/beast/insight/Counter.h>
#include <ripple/beast/insight/Event.h>
#include <ripple/beast/insight/Gauge.h>
#include <ripple/beast/insight/Hook.h>
#include <ripple/beast/insight/Meter.h>

#include <string>

namespace beast {
namespace insight {


class Collector
{
public:
    using ptr = std::shared_ptr <Collector>;

    virtual ~Collector() = 0;

    
    
    template <class Handler>
    Hook make_hook (Handler handler)
    {
        return make_hook (HookImpl::HandlerType (handler));
    }

    virtual Hook make_hook (HookImpl::HandlerType const& handler) = 0;
    

    
    
    virtual Counter make_counter (std::string const& name) = 0;

    Counter make_counter (std::string const& prefix, std::string const& name)
    {
        if (prefix.empty ())
            return make_counter (name);
        return make_counter (prefix + "." + name);
    }
    

    
    
    virtual Event make_event (std::string const& name) = 0;

    Event make_event (std::string const& prefix, std::string const& name)
    {
        if (prefix.empty ())
            return make_event (name);
        return make_event (prefix + "." + name);
    }
    

    
    
    virtual Gauge make_gauge (std::string const& name) = 0;

    Gauge make_gauge (std::string const& prefix, std::string const& name)
    {
        if (prefix.empty ())
            return make_gauge (name);
        return make_gauge (prefix + "." + name);
    }
    

    
    
    virtual Meter make_meter (std::string const& name) = 0;

    Meter make_meter (std::string const& prefix, std::string const& name)
    {
        if (prefix.empty ())
            return make_meter (name);
        return make_meter (prefix + "." + name);
    }
    
};

}
}

#endif








