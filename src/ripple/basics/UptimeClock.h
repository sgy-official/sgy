

#ifndef RIPPLE_BASICS_UPTIMETIMER_H_INCLUDED
#define RIPPLE_BASICS_UPTIMETIMER_H_INCLUDED

#include <atomic>
#include <chrono>
#include <ratio>
#include <thread>

namespace ripple {



class UptimeClock
{
public:
    using rep        = int;
    using period     = std::ratio<1>;
    using duration   = std::chrono::duration<rep, period>;
    using time_point = std::chrono::time_point<UptimeClock>;
    static constexpr bool is_steady = std::chrono::system_clock::is_steady;

    explicit UptimeClock() = default;

    static time_point now();  

private:
    static std::atomic<rep>  now_;
    static std::atomic<bool> stop_;

    struct update_thread
        : private std::thread
    {
        ~update_thread();
        update_thread(update_thread&&) = default;

        using std::thread::thread;
    };

    static update_thread start_clock();
};

} 

#endif








