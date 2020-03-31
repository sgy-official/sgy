

#ifndef RIPPLE_CORE_JOBTYPEDATA_H_INCLUDED
#define RIPPLE_CORE_JOBTYPEDATA_H_INCLUDED

#include <ripple/basics/Log.h>
#include <ripple/core/JobTypeInfo.h>
#include <ripple/beast/insight/Collector.h>

namespace ripple
{

struct JobTypeData
{
private:
    LoadMonitor m_load;

    
    beast::insight::Collector::ptr m_collector;

public:
    
    JobTypeInfo const& info;

    
    int waiting;

    
    int running;

    
    int deferred;

    
    beast::insight::Event dequeue;
    beast::insight::Event execute;

    JobTypeData (JobTypeInfo const& info_,
            beast::insight::Collector::ptr const& collector, Logs& logs) noexcept
        : m_load (logs.journal ("LoadMonitor"))
        , m_collector (collector)
        , info (info_)
        , waiting (0)
        , running (0)
        , deferred (0)
    {
        m_load.setTargetLatency (
            info.getAverageLatency (),
            info.getPeakLatency());

        if (!info.special ())
        {
            dequeue = m_collector->make_event (info.name () + "_q");
            execute = m_collector->make_event (info.name ());
        }
    }

    
    JobTypeData (JobTypeData const& other) = delete;
    JobTypeData& operator= (JobTypeData const& other) = delete;

    std::string name () const
    {
        return info.name ();
    }

    JobType type () const
    {
        return info.type ();
    }

    LoadMonitor& load ()
    {
        return m_load;
    }

    LoadMonitor::Stats stats ()
    {
        return m_load.getStats ();
    }
};

}

#endif








