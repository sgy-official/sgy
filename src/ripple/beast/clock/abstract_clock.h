

#ifndef BEAST_CHRONO_ABSTRACT_CLOCK_H_INCLUDED
#define BEAST_CHRONO_ABSTRACT_CLOCK_H_INCLUDED

#include <chrono>
#include <string>

namespace beast {


template <class Clock>
class abstract_clock
{
public:
    using rep = typename Clock::rep;
    using period = typename Clock::period;
    using duration = typename Clock::duration;
    using time_point = typename Clock::time_point;
    using clock_type = Clock;

    static bool const is_steady = Clock::is_steady;

    virtual ~abstract_clock() = default;
    abstract_clock() = default;
    abstract_clock(abstract_clock const&) = default;

    
    virtual time_point now() const = 0;
};


namespace detail {

template <class Facade, class Clock>
struct abstract_clock_wrapper
    : public abstract_clock<Facade>
{
    explicit abstract_clock_wrapper() = default;

    using typename abstract_clock<Facade>::duration;
    using typename abstract_clock<Facade>::time_point;

    time_point
    now() const override
    {
        return Clock::now();
    }
};

}



template<class Facade, class Clock = Facade>
abstract_clock<Facade>&
get_abstract_clock()
{
    static detail::abstract_clock_wrapper<Facade, Clock> clock;
    return clock;
}

}

#endif








