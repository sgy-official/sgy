

#ifndef BEAST_INSIGHT_EVENT_H_INCLUDED
#define BEAST_INSIGHT_EVENT_H_INCLUDED

#include <ripple/beast/insight/Base.h>
#include <ripple/beast/insight/EventImpl.h>

#include <ripple/basics/date.h>

#include <chrono>
#include <memory>

namespace beast {
namespace insight {


class Event : public Base
{
public:
    using value_type = EventImpl::value_type;

    
    Event ()
        { }

    
    explicit Event (std::shared_ptr <EventImpl> const& impl)
        : m_impl (impl)
        { }

    
    template <class Rep, class Period>
    void
    notify (std::chrono::duration <Rep, Period> const& value) const
    {
        using namespace std::chrono;
        if (m_impl)
            m_impl->notify (date::ceil <value_type> (value));
    }

    std::shared_ptr <EventImpl> const& impl () const
    {
        return m_impl;
    }

private:
    std::shared_ptr <EventImpl> m_impl;
};

}
}

#endif








