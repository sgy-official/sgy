

#ifndef BEAST_CHRONO_MANUAL_CLOCK_H_INCLUDED
#define BEAST_CHRONO_MANUAL_CLOCK_H_INCLUDED

#include <ripple/beast/clock/abstract_clock.h>
#include <cassert>

namespace beast {


template <class Clock>
class manual_clock
    : public abstract_clock<Clock>
{
public:
    using typename abstract_clock<Clock>::rep;
    using typename abstract_clock<Clock>::duration;
    using typename abstract_clock<Clock>::time_point;

private:
    time_point now_;

public:
    explicit
    manual_clock (time_point const& now = time_point(duration(0)))
        : now_(now)
    {
    }

    time_point
    now() const override
    {
        return now_;
    }

    
    void
    set (time_point const& when)
    {
        assert(! Clock::is_steady || when >= now_);
        now_ = when;
    }

    
    template <class Integer>
    void
    set(Integer seconds_from_epoch)
    {
        set(time_point(duration(
            std::chrono::seconds(seconds_from_epoch))));
    }

    
    template <class Rep, class Period>
    void
    advance(std::chrono::duration<Rep, Period> const& elapsed)
    {
        assert(! Clock::is_steady ||
            (now_ + elapsed) >= now_);
        now_ += elapsed;
    }

    
    manual_clock&
    operator++ ()
    {
        advance(std::chrono::seconds(1));
        return *this;
    }
};

}

#endif








