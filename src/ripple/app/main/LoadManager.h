

#ifndef RIPPLE_APP_MAIN_LOADMANAGER_H_INCLUDED
#define RIPPLE_APP_MAIN_LOADMANAGER_H_INCLUDED

#include <ripple/core/Stoppable.h>
#include <memory>
#include <mutex>
#include <thread>

namespace ripple {

class Application;


class LoadManager : public Stoppable
{
    LoadManager (Application& app, Stoppable& parent, beast::Journal journal);

public:
    LoadManager () = delete;
    LoadManager (LoadManager const&) = delete;
    LoadManager& operator=(LoadManager const&) = delete;

    
    ~LoadManager ();

    
    void activateDeadlockDetector ();

    
    void resetDeadlockDetector ();


    void onPrepare () override;

    void onStart () override;

    void onStop () override;

private:
    void run ();

private:
    Application& app_;
    beast::Journal journal_;

    std::thread thread_;
    std::mutex mutex_;          

    UptimeClock::time_point deadLock_;  
    bool armed_;
    bool stop_;

    friend
    std::unique_ptr<LoadManager>
    make_LoadManager(Application& app, Stoppable& parent, beast::Journal journal);
};

std::unique_ptr<LoadManager>
make_LoadManager (
    Application& app, Stoppable& parent, beast::Journal journal);

} 

#endif








