

#ifndef RIPPLE_CORE_JOBTYPEINFO_H_INCLUDED
#define RIPPLE_CORE_JOBTYPEINFO_H_INCLUDED

namespace ripple
{


class JobTypeInfo
{
private:
    JobType const m_type;
    std::string const m_name;

    
    int const m_limit;

    
    bool const m_special;

    
    std::chrono::milliseconds const m_avgLatency;
    std::chrono::milliseconds const m_peakLatency;

public:
    JobTypeInfo () = delete;

    JobTypeInfo (JobType type, std::string name, int limit,
            bool special, std::chrono::milliseconds avgLatency,
            std::chrono::milliseconds peakLatency)
        : m_type (type)
        , m_name (std::move(name))
        , m_limit (limit)
        , m_special (special)
        , m_avgLatency (avgLatency)
        , m_peakLatency (peakLatency)
    {

    }

    JobType type () const
    {
        return m_type;
    }

    std::string const& name () const
    {
        return m_name;
    }

    int limit () const
    {
        return m_limit;
    }

    bool special () const
    {
        return m_special;
    }

    std::chrono::milliseconds getAverageLatency () const
    {
        return m_avgLatency;
    }

    std::chrono::milliseconds getPeakLatency () const
    {
        return m_peakLatency;
    }
};

}

#endif








