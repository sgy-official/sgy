

#ifndef RIPPLE_NET_SNTPCLOCK_H_INCLUDED
#define RIPPLE_NET_SNTPCLOCK_H_INCLUDED

#include <ripple/beast/clock/abstract_clock.h>
#include <ripple/beast/utility/Journal.h>
#include <chrono>
#include <memory>
#include <string>
#include <vector>

namespace ripple {


class SNTPClock
    : public beast::abstract_clock<
        std::chrono::system_clock>
{
public:
    virtual
    void
    run (std::vector <std::string> const& servers) = 0;

    virtual
    duration
    offset() const = 0;
};

extern
std::unique_ptr<SNTPClock>
make_SNTPClock (beast::Journal);

} 

#endif








