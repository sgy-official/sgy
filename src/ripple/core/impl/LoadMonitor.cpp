

#include <ripple/basics/Log.h>
#include <ripple/basics/UptimeClock.h>
#include <ripple/basics/date.h>
#include <ripple/core/LoadMonitor.h>

namespace ripple {




LoadMonitor::Stats::Stats()
    : count (0)
    , latencyAvg (0)
    , latencyPeak (0)
    , isOverloaded (false)
{
}


LoadMonitor::LoadMonitor (beast::Journal j)
    : mCounts (0)
    , mLatencyEvents (0)
    , mLatencyMSAvg (0)
    , mLatencyMSPeak (0)
    , mTargetLatencyAvg (0)
    , mTargetLatencyPk (0)
    , mLastUpdate (UptimeClock::now())
    , j_ (j)
{
}

void LoadMonitor::update ()
{
    using namespace std::chrono_literals;
    auto now = UptimeClock::now();
    if (now == mLastUpdate) 
        return;

    if ((now < mLastUpdate) || (now > (mLastUpdate + 8s)))
    {
        mCounts = 0;
        mLatencyEvents = 0;
        mLatencyMSAvg = 0ms;
        mLatencyMSPeak = 0ms;
        mLastUpdate = now;
        return;
    }

    
    do
    {
        mLastUpdate += 1s;
        mCounts -= ((mCounts + 3) / 4);
        mLatencyEvents -= ((mLatencyEvents + 3) / 4);
        mLatencyMSAvg -= (mLatencyMSAvg / 4);
        mLatencyMSPeak -= (mLatencyMSPeak / 4);
    }
    while (mLastUpdate < now);
}

void LoadMonitor::addLoadSample (LoadEvent const& s)
{
    using namespace std::chrono;

    auto const total = s.runTime() + s.waitTime();
    auto const latency = total < 2ms ? 0ms : date::round<milliseconds>(total);

    if (latency > 500ms)
    {
        auto mj = (latency > 1s) ? j_.warn() : j_.info();
        JLOG (mj) << "Job: " << s.name() <<
            " run: " << date::round<milliseconds>(s.runTime()).count() <<
            "ms" << " wait: " <<
            date::round<milliseconds>(s.waitTime()).count() << "ms";
    }

    addSamples (1, latency);
}


void LoadMonitor::addSamples (int count, std::chrono::milliseconds latency)
{
    std::lock_guard<std::mutex> sl (mutex_);

    update ();
    mCounts += count;
    mLatencyEvents += count;
    mLatencyMSAvg += latency;
    mLatencyMSPeak += latency;

    auto const latencyPeak = mLatencyEvents * latency * 4 / count;

    if (mLatencyMSPeak < latencyPeak)
        mLatencyMSPeak = latencyPeak;
}

void LoadMonitor::setTargetLatency (std::chrono::milliseconds avg,
                                    std::chrono::milliseconds pk)
{
    mTargetLatencyAvg  = avg;
    mTargetLatencyPk = pk;
}

bool LoadMonitor::isOverTarget (std::chrono::milliseconds avg,
                                std::chrono::milliseconds peak)
{
    using namespace std::chrono_literals;
    return (mTargetLatencyPk > 0ms && (peak > mTargetLatencyPk)) ||
           (mTargetLatencyAvg > 0ms && (avg > mTargetLatencyAvg));
}

bool LoadMonitor::isOver ()
{
    std::lock_guard<std::mutex> sl (mutex_);

    update ();

    if (mLatencyEvents == 0)
        return 0;

    return isOverTarget (mLatencyMSAvg / (mLatencyEvents * 4), mLatencyMSPeak / (mLatencyEvents * 4));
}

LoadMonitor::Stats LoadMonitor::getStats ()
{
    using namespace std::chrono_literals;
    Stats stats;

    std::lock_guard<std::mutex> sl (mutex_);

    update ();

    stats.count = mCounts / 4;

    if (mLatencyEvents == 0)
    {
        stats.latencyAvg = 0ms;
        stats.latencyPeak = 0ms;
    }
    else
    {
        stats.latencyAvg = mLatencyMSAvg / (mLatencyEvents * 4);
        stats.latencyPeak = mLatencyMSPeak / (mLatencyEvents * 4);
    }

    stats.isOverloaded = isOverTarget (stats.latencyAvg, stats.latencyPeak);

    return stats;
}

} 
























