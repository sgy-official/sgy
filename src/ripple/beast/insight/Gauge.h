

#ifndef BEAST_INSIGHT_GAUGE_H_INCLUDED
#define BEAST_INSIGHT_GAUGE_H_INCLUDED

#include <ripple/beast/insight/Base.h>
#include <ripple/beast/insight/GaugeImpl.h>

#include <memory>

namespace beast {
namespace insight {


class Gauge : public Base
{
public:
    using value_type = GaugeImpl::value_type;
    using difference_type = GaugeImpl::difference_type;

    
    Gauge ()
    {
    }

    
    explicit Gauge (std::shared_ptr <GaugeImpl> const& impl)
        : m_impl (impl)
    {
    }

    
    
    void set (value_type value) const
    {
        if (m_impl)
            m_impl->set (value);
    }

    Gauge const& operator= (value_type value) const
        { set (value); return *this; }
    

    
    
    void increment (difference_type amount) const
    {
        if (m_impl)
            m_impl->increment (amount);
    }

    Gauge const&
    operator+= (difference_type amount) const
    {
        increment (amount);
        return *this;
    }

    Gauge const&
    operator-= (difference_type amount) const
    {
        increment (-amount);
        return *this;
    }

    Gauge const&
    operator++ () const
    {
        increment (1);
        return *this;
    }

    Gauge const&
    operator++ (int) const
    {
        increment (1);
        return *this;
    }

    Gauge const&
    operator-- () const
    {
        increment (-1);
        return *this;
    }

    Gauge const&
    operator-- (int) const
    {
        increment (-1);
        return *this;
    }
    

    std::shared_ptr <GaugeImpl> const&
    impl () const
    {
        return m_impl;
    }

private:
    std::shared_ptr <GaugeImpl> m_impl;
};

}
}

#endif








