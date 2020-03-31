

#ifndef RIPPLE_CORE_LOADEVENT_H_INCLUDED
#define RIPPLE_CORE_LOADEVENT_H_INCLUDED

#include <chrono>
#include <memory>
#include <string>

namespace ripple {

class LoadMonitor;

class LoadEvent
{
public:
    LoadEvent (LoadMonitor& monitor,
               std::string const& name,
               bool shouldStart);
    LoadEvent(LoadEvent const&) = delete;

    ~LoadEvent ();

    std::string const&
    name () const;

    std::chrono::steady_clock::duration
    waitTime() const;

    std::chrono::steady_clock::duration
    runTime() const;

    void setName (std::string const& name);

    void start ();

    void stop ();

private:
    LoadMonitor& monitor_;

    bool running_;

    std::string name_;

    std::chrono::steady_clock::time_point mark_;

    std::chrono::steady_clock::duration timeWaiting_;
    std::chrono::steady_clock::duration timeRunning_;
};

} 

#endif








