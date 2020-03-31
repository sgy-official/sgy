

#include <ripple/core/impl/Workers.h>
#include <ripple/basics/PerfLog.h>
#include <ripple/beast/core/CurrentThreadName.h>
#include <cassert>

namespace ripple {

Workers::Workers (
    Callback& callback,
    perf::PerfLog& perfLog,
    std::string const& threadNames,
    int numberOfThreads)
        : m_callback (callback)
        , perfLog_ (perfLog)
        , m_threadNames (threadNames)
        , m_allPaused (true)
        , m_semaphore (0)
        , m_numberOfThreads (0)
        , m_activeCount (0)
        , m_pauseCount (0)
        , m_runningTaskCount (0)
{
    setNumberOfThreads (numberOfThreads);
}

Workers::~Workers ()
{
    pauseAllThreadsAndWait ();

    deleteWorkers (m_everyone);
}

int Workers::getNumberOfThreads () const noexcept
{
    return m_numberOfThreads;
}

void Workers::setNumberOfThreads (int numberOfThreads)
{
    static int instance {0};
    if (m_numberOfThreads != numberOfThreads)
    {
        perfLog_.resizeJobs(numberOfThreads);

        if (numberOfThreads > m_numberOfThreads)
        {
            int const amount = numberOfThreads - m_numberOfThreads;

            for (int i = 0; i < amount; ++i)
            {
                Worker* worker = m_paused.pop_front ();

                if (worker != nullptr)
                {
                    worker->notify ();
                }
                else
                {
                    worker = new Worker (*this, m_threadNames, instance++);
                    m_everyone.push_front (worker);
                }
            }
        }
        else
        {
            int const amount = m_numberOfThreads - numberOfThreads;

            for (int i = 0; i < amount; ++i)
            {
                ++m_pauseCount;

                m_semaphore.notify ();
            }
        }

        m_numberOfThreads = numberOfThreads;
    }
}

void Workers::pauseAllThreadsAndWait ()
{
    setNumberOfThreads (0);

    std::unique_lock<std::mutex> lk{m_mut};
    m_cv.wait(lk, [this]{return m_allPaused;});
    lk.unlock();

    assert (numberOfCurrentlyRunningTasks () == 0);
}

void Workers::addTask ()
{
    m_semaphore.notify ();
}

int Workers::numberOfCurrentlyRunningTasks () const noexcept
{
    return m_runningTaskCount.load ();
}

void Workers::deleteWorkers (beast::LockFreeStack <Worker>& stack)
{
    for (;;)
    {
        Worker* const worker = stack.pop_front ();

        if (worker != nullptr)
        {
            delete worker;
        }
        else
        {
            break;
        }
    }
}


Workers::Worker::Worker (Workers& workers, std::string const& threadName,
    int const instance)
    : m_workers {workers}
    , threadName_ {threadName}
    , instance_ {instance}
    , wakeCount_ {0}
    , shouldExit_ {false}
{
    thread_ = std::thread {&Workers::Worker::run, this};
}

Workers::Worker::~Worker ()
{
    {
        std::lock_guard <std::mutex> lock {mutex_};
        ++wakeCount_;
        shouldExit_ = true;
    }

    wakeup_.notify_one();
    thread_.join();
}

void Workers::Worker::notify ()
{
    std::lock_guard <std::mutex> lock {mutex_};
    ++wakeCount_;
    wakeup_.notify_one();
}

void Workers::Worker::run ()
{
    bool shouldExit = true;
    do
    {
        if (++m_workers.m_activeCount == 1)
        {
            std::lock_guard<std::mutex> lk{m_workers.m_mut};
            m_workers.m_allPaused = false;
        }

        for (;;)
        {
            beast::setCurrentThreadName (threadName_);

            m_workers.m_semaphore.wait ();

            int pauseCount = m_workers.m_pauseCount.load ();

            if (pauseCount > 0)
            {
                pauseCount = --m_workers.m_pauseCount;

                if (pauseCount >= 0)
                {
                    break;
                }
                else
                {
                    ++m_workers.m_pauseCount;
                }
            }

            ++m_workers.m_runningTaskCount;
            m_workers.m_callback.processTask (instance_);
            --m_workers.m_runningTaskCount;
        }

        m_workers.m_paused.push_front (this);

        if (--m_workers.m_activeCount == 0)
        {
            std::lock_guard<std::mutex> lk{m_workers.m_mut};
            m_workers.m_allPaused = true;
            m_workers.m_cv.notify_all();
        }

        beast::setCurrentThreadName ("(" + threadName_ + ")");

        {
            std::unique_lock <std::mutex> lock {mutex_};
            wakeup_.wait (lock, [this] {return this->wakeCount_ > 0;});

            shouldExit = shouldExit_;
            --wakeCount_;
        }
    } while (! shouldExit);
}

} 
























