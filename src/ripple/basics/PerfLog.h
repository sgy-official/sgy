

#ifndef RIPPLE_BASICS_PERFLOG_H
#define RIPPLE_BASICS_PERFLOG_H

#include <ripple/core/JobTypes.h>
#include <boost/filesystem.hpp>
#include <ripple/json/json_value.h>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>

namespace beast { class Journal; }

namespace ripple {
namespace perf {



class PerfLog
{
public:
    using steady_clock = std::chrono::steady_clock;
    using system_clock = std::chrono::system_clock;
    using steady_time_point = std::chrono::time_point<steady_clock>;
    using system_time_point = std::chrono::time_point<system_clock>;
    using seconds = std::chrono::seconds;
    using milliseconds = std::chrono::milliseconds;
    using microseconds = std::chrono::microseconds;

    
    struct Setup
    {
        boost::filesystem::path perfLog;
        milliseconds logInterval {seconds(1)};
    };

    virtual ~PerfLog() = default;

    
    virtual void rpcStart(std::string const& method,
        std::uint64_t requestId) = 0;

    
    virtual void rpcFinish(std::string const& method,
        std::uint64_t requestId) = 0;

    
    virtual void rpcError(std::string const& method,
        std::uint64_t requestId) = 0;

    
    virtual void jobQueue(JobType const type) = 0;

    
    virtual void jobStart(JobType const type,
        microseconds dur,
        steady_time_point startTime,
        int instance) = 0;

    
    virtual void jobFinish(JobType const type,
        microseconds dur, int instance) = 0;

    
    virtual Json::Value countersJson() const = 0;

    
    virtual Json::Value currentJson() const = 0;

    
    virtual void resizeJobs(int const resize) = 0;

    
    virtual void rotate() = 0;
};

} 

class Section;
class Stoppable;

namespace perf {

PerfLog::Setup setup_PerfLog(Section const& section,
    boost::filesystem::path const& configDir);

std::unique_ptr<PerfLog> make_PerfLog(
    PerfLog::Setup const& setup,
    Stoppable& parent,
    beast::Journal journal,
    std::function<void()>&& signalStop);

} 
} 

#endif 








