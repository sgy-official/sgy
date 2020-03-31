

#ifndef RIPPLE_BASICS_PERFLOGIMP_H
#define RIPPLE_BASICS_PERFLOGIMP_H

#include <ripple/basics/chrono.h>
#include <ripple/basics/PerfLog.h>
#include <ripple/beast/utility/Journal.h>
#include <ripple/core/Stoppable.h>
#include <ripple/protocol/jss.h>
#include <ripple/rpc/impl/Handler.h>
#include <boost/asio/ip/host_name.hpp>
#include <condition_variable>
#include <cstdint>
#include <fstream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ripple {
namespace perf {


class PerfLogImp
    : public PerfLog, Stoppable
{
    
    struct Counters
    {
    public:
        using MethodStart = std::pair<char const*, steady_time_point>;
        
        struct Rpc
        {
            struct Sync
            {
                std::uint64_t started {0};
                std::uint64_t finished {0};
                std::uint64_t errored {0};
                microseconds duration {0};
            };

            Sync sync;
            mutable std::mutex mut;

            Rpc() = default;

            Rpc(Rpc const& orig)
                : sync (orig.sync)
            {}
        };

        
        struct Jq
        {
            struct Sync
            {
                std::uint64_t queued {0};
                std::uint64_t started {0};
                std::uint64_t finished {0};
                microseconds queuedDuration {0};
                microseconds runningDuration {0};
            };

            Sync sync;
            std::string const label;
            mutable std::mutex mut;

            Jq(std::string const& labelArg)
                : label (labelArg)
            {}

            Jq(Jq const& orig)
            : sync (orig.sync)
            , label (orig.label)
            {}
        };

        std::unordered_map<std::string, Rpc> rpc_;
        std::unordered_map<std::underlying_type_t<JobType>, Jq> jq_;
        std::vector<std::pair<JobType, steady_time_point>> jobs_;
        int workers_ {0};
        mutable std::mutex jobsMutex_;
        std::unordered_map<std::uint64_t, MethodStart> methods_;
        mutable std::mutex methodsMutex_;

        Counters(std::vector<char const*> const& labels,
            JobTypes const& jobTypes);
        Json::Value countersJson() const;
        Json::Value currentJson() const;
    };

    Setup const setup_;
    beast::Journal j_;
    std::function<void()> const signalStop_;
    Counters counters_ {ripple::RPC::getHandlerNames(), JobTypes::instance()};
    std::ofstream logFile_;
    std::thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;
    system_time_point lastLog_;
    std::string const hostname_ {boost::asio::ip::host_name()};
    bool stop_ {false};
    bool rotate_ {false};

    void openLog();
    void run();
    void report();
    void rpcEnd(std::string const& method,
        std::uint64_t const requestId,
        bool finish);

public:
    PerfLogImp(Setup const& setup,
        Stoppable& parent,
        beast::Journal journal,
        std::function<void()>&& signalStop);

    ~PerfLogImp() override;

    void rpcStart(
        std::string const& method, std::uint64_t const requestId) override;

    void rpcFinish(
        std::string const& method,
        std::uint64_t const requestId) override
    {
        rpcEnd(method, requestId, true);
    }

    void rpcError(std::string const& method,
        std::uint64_t const requestId) override
    {
        rpcEnd(method, requestId, false);
    }

    void jobQueue(JobType const type) override;
    void jobStart(
        JobType const type,
        microseconds dur,
        steady_time_point startTime,
        int instance) override;
    void jobFinish(
        JobType const type,
        microseconds dur,
        int instance) override;

    Json::Value
    countersJson() const override
    {
        return counters_.countersJson();
    }

    Json::Value
    currentJson() const override
    {
        return counters_.currentJson();
    }

    void resizeJobs(int const resize) override;
    void rotate() override;

    void onPrepare() override {}

    void onStart() override;
    void onStop() override;
    void onChildrenStopped() override;
};

} 
} 

#endif 








