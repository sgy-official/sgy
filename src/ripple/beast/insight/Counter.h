

#ifndef BEAST_INSIGHT_COUNTER_H_INCLUDED
#define BEAST_INSIGHT_COUNTER_H_INCLUDED

#include <ripple/beast/insight/Base.h>
#include <ripple/beast/insight/CounterImpl.h>

#include <memory>

namespace beast {
namespace insight {


class Counter : public Base
{
public:
    using value_type = CounterImpl::value_type;

    
    Counter ()
    {
    }

    
    explicit Counter (std::shared_ptr <CounterImpl> const& impl)
        : m_impl (impl)
    {
    }

    
    
    void
    increment (value_type amount) const
    {
        if (m_impl)
            m_impl->increment (amount);
    }

    Counter const&
    operator+= (value_type amount) const
    {
        increment (amount);
        return *this;
    }

    Counter const&
    operator-= (value_type amount) const
    {
        increment (-amount);
        return *this;
    }

    Counter const&
    operator++ () const
    {
        increment (1);
        return *this;
    }

    Counter const&
    operator++ (int) const
    {
        increment (1);
        return *this;
    }

    Counter const&
    operator-- () const
    {
        increment (-1);
        return *this;
    }

    Counter const&
    operator-- (int) const
    {
        increment (-1);
        return *this;
    }
    

    std::shared_ptr <CounterImpl> const&
    impl () const
    {
        return m_impl;
    }

private:
    std::shared_ptr <CounterImpl> m_impl;
};

}
}

#endif








