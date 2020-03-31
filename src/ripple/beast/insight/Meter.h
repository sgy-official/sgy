

#ifndef BEAST_INSIGHT_METER_H_INCLUDED
#define BEAST_INSIGHT_METER_H_INCLUDED

#include <memory>

#include <ripple/beast/insight/Base.h>
#include <ripple/beast/insight/MeterImpl.h>

namespace beast {
namespace insight {


class Meter : public Base
{
public:
    using value_type = MeterImpl::value_type;

    
    Meter ()
        { }

    
    explicit Meter (std::shared_ptr <MeterImpl> const& impl)
        : m_impl (impl)
        { }

    
    
    void increment (value_type amount) const
    {
        if (m_impl)
            m_impl->increment (amount);
    }

    Meter const& operator+= (value_type amount) const
    {
        increment (amount);
        return *this;
    }

    Meter const& operator++ () const
    {
        increment (1);
        return *this;
    }

    Meter const& operator++ (int) const
    {
        increment (1);
        return *this;
    }
    

    std::shared_ptr <MeterImpl> const& impl () const
    {
        return m_impl;
    }

private:
    std::shared_ptr <MeterImpl> m_impl;
};

}
}

#endif








