

#ifndef RIPPLE_CORE_TIMEKEEPER_H_INCLUDED
#define RIPPLE_CORE_TIMEKEEPER_H_INCLUDED

#include <ripple/beast/clock/abstract_clock.h>
#include <ripple/beast/utility/Journal.h>
#include <ripple/basics/chrono.h>
#include <string>
#include <vector>

namespace ripple {


class TimeKeeper
    : public beast::abstract_clock<NetClock>
{
public:
    virtual ~TimeKeeper() = default;

    
    virtual
    void
    run (std::vector<std::string> const& servers) = 0;

    
    virtual
    time_point
    now() const override = 0;

    
    virtual
    time_point
    closeTime() const = 0;

    
    virtual
    void
    adjustCloseTime (std::chrono::duration<std::int32_t> amount) = 0;

    virtual
    std::chrono::duration<std::int32_t>
    nowOffset() const = 0;

    virtual
    std::chrono::duration<std::int32_t>
    closeOffset() const = 0;
};

extern
std::unique_ptr<TimeKeeper>
make_TimeKeeper(beast::Journal j);

} 

#endif








