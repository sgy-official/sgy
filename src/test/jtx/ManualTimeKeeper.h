

#ifndef RIPPLE_TEST_MANUALTIMEKEEPER_H_INCLUDED
#define RIPPLE_TEST_MANUALTIMEKEEPER_H_INCLUDED

#include <ripple/core/TimeKeeper.h>
#include <mutex>

namespace ripple {
namespace test {

class ManualTimeKeeper : public TimeKeeper
{
public:
    ManualTimeKeeper();

    void
    run (std::vector<std::string> const& servers) override;

    time_point
    now() const override;
    
    time_point
    closeTime() const override;

    void
    adjustCloseTime (std::chrono::duration<std::int32_t> amount) override;

    std::chrono::duration<std::int32_t>
    nowOffset() const override;

    std::chrono::duration<std::int32_t>
    closeOffset() const override;

    void
    set (time_point now);

private:
    static
    time_point
    adjust (std::chrono::system_clock::time_point when);

    std::mutex mutable mutex_;
    std::chrono::duration<std::int32_t> closeOffset_;
    time_point now_;
};

} 
} 

#endif








