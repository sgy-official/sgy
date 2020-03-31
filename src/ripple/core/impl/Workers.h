

#ifndef RIPPLE_CORE_WORKERS_H_INCLUDED
#define RIPPLE_CORE_WORKERS_H_INCLUDED

#include <ripple/core/impl/semaphore.h>
#include <ripple/beast/core/LockFreeStack.h>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

namespace ripple {

namespace perf
{
    class PerfLog;
}


class Workers
{
public:
    
    struct Callback
    {
        virtual ~Callback () = default;
        Callback() = default;
        Callback(Callback const&) = delete;
        Callback& operator=(Callback const&) = delete;

        
        virtual void processTask (int instance) = 0;
    };

    
    explicit Workers (Callback& callback,
                      perf::PerfLog& perfLog,
                      std::string const& threadNames = "Worker",
                      int numberOfThreads =
                         static_cast<int>(std::thread::hardware_concurrency()));

    ~Workers ();

    
    int getNumberOfThreads () const noexcept;

    
    void setNumberOfThreads (int numberOfThreads);

    
    void pauseAllThreadsAndWait ();

    
    void addTask ();

    
    int numberOfCurrentlyRunningTasks () const noexcept;


private:
    struct PausedTag
    {
        explicit PausedTag() = default;
    };

    
    class Worker
        : public beast::LockFreeStack <Worker>::Node
        , public beast::LockFreeStack <Worker, PausedTag>::Node
    {
    public:
        Worker (Workers& workers,
            std::string const& threadName,
            int const instance);

        ~Worker ();

        void notify ();

    private:
        void run ();

    private:
        Workers& m_workers;
        std::string const threadName_;
        int const instance_;

        std::thread thread_;
        std::mutex mutex_;
        std::condition_variable wakeup_;
        int wakeCount_;               
        bool shouldExit_;
    };

private:
    static void deleteWorkers (beast::LockFreeStack <Worker>& stack);

private:
    Callback& m_callback;
    perf::PerfLog& perfLog_;
    std::string m_threadNames;                   
    std::condition_variable m_cv;                
    std::mutex              m_mut;
    bool                    m_allPaused;
    semaphore m_semaphore;                       
    int m_numberOfThreads;                       
    std::atomic <int> m_activeCount;             
    std::atomic <int> m_pauseCount;              
    std::atomic <int> m_runningTaskCount;        
    beast::LockFreeStack <Worker> m_everyone;           
    beast::LockFreeStack <Worker, PausedTag> m_paused;  
};

} 

#endif








