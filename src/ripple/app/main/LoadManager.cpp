

#include <ripple/app/main/LoadManager.h>
#include <ripple/app/main/Application.h>
#include <ripple/app/misc/LoadFeeTrack.h>
#include <ripple/app/misc/NetworkOPs.h>
#include <ripple/basics/UptimeClock.h>
#include <ripple/json/to_string.h>
#include <ripple/beast/core/CurrentThreadName.h>
#include <memory>
#include <mutex>
#include <thread>

namespace ripple {

LoadManager::LoadManager (
    Application& app, Stoppable& parent, beast::Journal journal)
    : Stoppable ("LoadManager", parent)
    , app_ (app)
    , journal_ (journal)
    , deadLock_ ()
    , armed_ (false)
    , stop_ (false)
{
}

LoadManager::~LoadManager ()
{
    try
    {
        onStop ();
    }
    catch (std::exception const& ex)
    {
        JLOG(journal_.warn()) << "std::exception in ~LoadManager.  "
            << ex.what();
    }
}


void LoadManager::activateDeadlockDetector ()
{
    std::lock_guard<std::mutex> sl (mutex_);
    armed_ = true;
}

void LoadManager::resetDeadlockDetector ()
{
    auto const elapsedSeconds = UptimeClock::now();
    std::lock_guard<std::mutex> sl (mutex_);
    deadLock_ = elapsedSeconds;
}


void LoadManager::onPrepare ()
{
}

void LoadManager::onStart ()
{
    JLOG(journal_.debug()) << "Starting";
    assert (! thread_.joinable());

    thread_ = std::thread {&LoadManager::run, this};
}

void LoadManager::onStop ()
{
    if (thread_.joinable())
    {
        JLOG(journal_.debug()) << "Stopping";
        {
            std::lock_guard<std::mutex> sl (mutex_);
            stop_ = true;
        }
        thread_.join();
    }
    stopped ();
}


void LoadManager::run ()
{
    beast::setCurrentThreadName ("LoadManager");

    using namespace std::chrono_literals;
    using clock_type = std::chrono::system_clock;

    auto t = clock_type::now();
    bool stop = false;

    while (! (stop || isStopping ()))
    {
        {
            std::unique_lock<std::mutex> sl (mutex_);
            auto const deadLock = deadLock_;
            auto const armed = armed_;
            stop = stop_;
            sl.unlock();

            auto const timeSpentDeadlocked = UptimeClock::now() - deadLock;

            auto const reportingIntervalSeconds = 10s;
            if (armed && (timeSpentDeadlocked >= reportingIntervalSeconds))
            {
                if ((timeSpentDeadlocked % reportingIntervalSeconds) == 0s)
                {
                    JLOG(journal_.warn())
                        << "Server stalled for "
                        << timeSpentDeadlocked.count() << " seconds.";
                }

                constexpr auto deadlockTimeLimit = 90s;
                assert (timeSpentDeadlocked < deadlockTimeLimit);

                if (timeSpentDeadlocked >= deadlockTimeLimit)
                    LogicError("Deadlock detected");
            }
        }

        bool change = false;
        if (app_.getJobQueue ().isOverloaded ())
        {
            JLOG(journal_.info()) << app_.getJobQueue ().getJson (0);
            change = app_.getFeeTrack ().raiseLocalFee ();
        }
        else
        {
            change = app_.getFeeTrack ().lowerLocalFee ();
        }

        if (change)
        {
            app_.getOPs ().reportFeeChange ();
        }

        t += 1s;
        auto const duration = t - clock_type::now();

        if ((duration < 0s) || (duration > 1s))
        {
            JLOG(journal_.warn()) << "time jump";
            t = clock_type::now();
        }
        else
        {
            alertable_sleep_until(t);
        }
    }

    stopped ();
}


std::unique_ptr<LoadManager>
make_LoadManager (Application& app,
    Stoppable& parent, beast::Journal journal)
{
    return std::unique_ptr<LoadManager>{new LoadManager{app, parent, journal}};
}

} 
























