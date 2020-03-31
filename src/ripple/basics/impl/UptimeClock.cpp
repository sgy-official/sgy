

#include <ripple/basics/UptimeClock.h>

namespace ripple {

std::atomic<UptimeClock::rep>  UptimeClock::now_{0};      
std::atomic<bool>              UptimeClock::stop_{false}; 

UptimeClock::update_thread::~update_thread()
{
    if (joinable())
    {
        stop_ = true;
        join();
    }
}

UptimeClock::update_thread
UptimeClock::start_clock()
{
    return update_thread{[]
                         {
                             using namespace std;
                             using namespace std::chrono;

                             auto next = system_clock::now() + 1s;
                             while (!stop_)
                             {
                                 this_thread::sleep_until(next);
                                 next += 1s;
                                 ++now_;
                             }
                         }};
}


UptimeClock::time_point
UptimeClock::now()
{
    static const auto init = start_clock();

    return time_point{duration{now_}};
}

} 
























