

#ifndef RIPPLE_CORE_JOBQUEUE_H_INCLUDED
#define RIPPLE_CORE_JOBQUEUE_H_INCLUDED

#include <ripple/basics/LocalValue.h>
#include <ripple/basics/win32_workaround.h>
#include <ripple/core/JobTypes.h>
#include <ripple/core/JobTypeData.h>
#include <ripple/core/Stoppable.h>
#include <ripple/core/impl/Workers.h>
#include <ripple/json/json_value.h>
#include <boost/coroutine/all.hpp>

namespace ripple {

namespace perf
{
    class PerfLog;
}

class Logs;
struct Coro_create_t
{
    explicit Coro_create_t() = default;
};


class JobQueue
    : public Stoppable
    , private Workers::Callback
{
public:
    
    class Coro : public std::enable_shared_from_this<Coro>
    {
    private:
        detail::LocalValues lvs_;
        JobQueue& jq_;
        JobType type_;
        std::string name_;
        bool running_;
        std::mutex mutex_;
        std::mutex mutex_run_;
        std::condition_variable cv_;
        boost::coroutines::asymmetric_coroutine<void>::pull_type coro_;
        boost::coroutines::asymmetric_coroutine<void>::push_type* yield_;
    #ifndef NDEBUG
        bool finished_ = false;
    #endif

    public:
        template <class F>
        Coro(Coro_create_t, JobQueue&, JobType,
            std::string const&, F&&);

        Coro(Coro const&) = delete;
        Coro& operator= (Coro const&) = delete;

        ~Coro();

        
        void yield() const;

        
        bool post();

        
        void resume();

        
        bool runnable() const;

        
        void expectEarlyExit();

        
        void join();
    };

    using JobFunction = std::function <void(Job&)>;

    JobQueue (beast::insight::Collector::ptr const& collector,
        Stoppable& parent, beast::Journal journal, Logs& logs,
        perf::PerfLog& perfLog);
    ~JobQueue ();

    
    template <typename JobHandler,
        typename = std::enable_if_t<std::is_same<decltype(
            std::declval<JobHandler&&>()(std::declval<Job&>())), void>::value>>
    bool addJob (JobType type,
        std::string const& name, JobHandler&& jobHandler)
    {
        if (auto optionalCountedJob =
            Stoppable::jobCounter().wrap (std::forward<JobHandler>(jobHandler)))
        {
            return addRefCountedJob (
                type, name, std::move (*optionalCountedJob));
        }
        return false;
    }

    
    template <class F>
    std::shared_ptr<Coro> postCoro (JobType t, std::string const& name, F&& f);

    
    int getJobCount (JobType t) const;

    
    int getJobCountTotal (JobType t) const;

    
    int getJobCountGE (JobType t) const;

    
    void setThreadCount (int c, bool const standaloneMode);

    
    std::unique_ptr <LoadEvent>
    makeLoadEvent (JobType t, std::string const& name);

    
    void addLoadEvents (JobType t, int count, std::chrono::milliseconds elapsed);

    bool isOverloaded ();

    Json::Value getJson (int c = 0);

    
    void
    rendezvous();

private:
    friend class Coro;

    using JobDataMap = std::map <JobType, JobTypeData>;

    beast::Journal m_journal;
    mutable std::mutex m_mutex;
    std::uint64_t m_lastJob;
    std::set <Job> m_jobSet;
    JobDataMap m_jobData;
    JobTypeData m_invalidJobData;

    int m_processCount;

    int nSuspend_ = 0;

    Workers m_workers;
    Job::CancelCallback m_cancelCallback;

    perf::PerfLog& perfLog_;
    beast::insight::Collector::ptr m_collector;
    beast::insight::Gauge job_count;
    beast::insight::Hook hook;

    std::condition_variable cv_;

    void collect();
    JobTypeData& getJobTypeData (JobType type);

    void onStop() override;

    void checkStopped (std::lock_guard <std::mutex> const& lock);

    bool addRefCountedJob (
        JobType type, std::string const& name, JobFunction const& func);

    void queueJob (Job const& job, std::lock_guard <std::mutex> const& lock);

    void getNextJob (Job& job);

    void finishJob (JobType type);

    void processTask (int instance) override;

    int getJobLimit (JobType type);

    void onChildrenStopped () override;
};



} 

#include <ripple/core/Coro.ipp>

namespace ripple {

template <class F>
std::shared_ptr<JobQueue::Coro>
JobQueue::postCoro (JobType t, std::string const& name, F&& f)
{
    
    auto coro = std::make_shared<Coro>(
        Coro_create_t{}, *this, t, name, std::forward<F>(f));
    if (! coro->post())
    {
        coro->expectEarlyExit();
        coro.reset();
    }
    return coro;
}

}

#endif








