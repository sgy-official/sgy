

#include <test/jtx/ManualTimeKeeper.h>

namespace ripple {
namespace test {

using namespace std::chrono_literals;

ManualTimeKeeper::ManualTimeKeeper()
    : closeOffset_ {}
    , now_ (0s)
{
}

void
ManualTimeKeeper::run (std::vector<std::string> const& servers)
{
}

auto
ManualTimeKeeper::now() const ->
    time_point
{
    std::lock_guard<std::mutex> lock(mutex_);
    return now_;
}
    
auto
ManualTimeKeeper::closeTime() const ->
    time_point
{
    std::lock_guard<std::mutex> lock(mutex_);
    return now_ + closeOffset_;
}

void
ManualTimeKeeper::adjustCloseTime(
    std::chrono::duration<std::int32_t> amount)
{
    using namespace std::chrono;
    auto const s = amount.count();
    std::lock_guard<std::mutex> lock(mutex_);
    if (s > 1)
        closeOffset_ += seconds((s + 3) / 4);
    else if (s < -1)
        closeOffset_ += seconds((s - 3) / 4);
    else
        closeOffset_ = (closeOffset_ * 3) / 4;
}

std::chrono::duration<std::int32_t>
ManualTimeKeeper::nowOffset() const
{
    return {};
}

std::chrono::duration<std::int32_t>
ManualTimeKeeper::closeOffset() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return closeOffset_;
}

void
ManualTimeKeeper::set (time_point now)
{
    std::lock_guard<std::mutex> lock(mutex_);
    now_ = now;
}

auto
ManualTimeKeeper::adjust(
        std::chrono::system_clock::time_point when) ->
    time_point
{
    return time_point(
        std::chrono::duration_cast<duration>(
            when.time_since_epoch() -
                days(10957)));
}
} 
} 
























