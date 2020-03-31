

#include <ripple/core/JobQueue.h>
#include <ripple/beast/unit_test.h>
#include <test/jtx/Env.h>

namespace ripple {
namespace test {


class JobQueue_test : public beast::unit_test::suite
{
    void testAddJob()
    {
        jtx::Env env {*this};

        JobQueue& jQueue = env.app().getJobQueue();
        {
            std::atomic<bool> jobRan {false};
            BEAST_EXPECT (jQueue.addJob (jtCLIENT, "JobAddTest1",
                [&jobRan] (Job&) { jobRan = true; }) == true);

            while (jobRan == false);
        }
        {
            using namespace std::chrono_literals;
            beast::Journal j {env.app().journal ("JobQueue_test")};
            JobCounter& jCounter = jQueue.jobCounter();
            jCounter.join("JobQueue_test", 1s, j);

            bool unprotected;
            BEAST_EXPECT (jQueue.addJob (jtCLIENT, "JobAddTest2",
                [&unprotected] (Job&) { unprotected = false; }) == false);
        }
    }

    void testPostCoro()
    {
        jtx::Env env {*this};

        JobQueue& jQueue = env.app().getJobQueue();
        {
            std::atomic<int> yieldCount {0};
            auto const coro = jQueue.postCoro (jtCLIENT, "PostCoroTest1",
                [&yieldCount] (std::shared_ptr<JobQueue::Coro> const& coroCopy)
                {
                    while (++yieldCount < 4)
                        coroCopy->yield();
                });
            BEAST_EXPECT (coro != nullptr);

            while (yieldCount == 0);

            int old = yieldCount;
            while (coro->runnable())
            {
                BEAST_EXPECT (coro->post());
                while (old == yieldCount) { }
                coro->join();
                BEAST_EXPECT (++old == yieldCount);
            }
            BEAST_EXPECT (yieldCount == 4);
        }
        {
            int yieldCount {0};
            auto const coro = jQueue.postCoro (jtCLIENT, "PostCoroTest2",
                [&yieldCount] (std::shared_ptr<JobQueue::Coro> const& coroCopy)
                {
                    while (++yieldCount < 4)
                        coroCopy->yield();
                });
            if (! coro)
            {
                BEAST_EXPECT (false);
                return;
            }

            coro->join();

            int old = yieldCount;
            while (coro->runnable())
            {
                coro->resume(); 
                BEAST_EXPECT (++old == yieldCount);
            }
            BEAST_EXPECT (yieldCount == 4);
        }
        {
            using namespace std::chrono_literals;
            beast::Journal j {env.app().journal ("JobQueue_test")};
            JobCounter& jCounter = jQueue.jobCounter();
            jCounter.join("JobQueue_test", 1s, j);

            bool unprotected;
            auto const coro = jQueue.postCoro (jtCLIENT, "PostCoroTest3",
                [&unprotected] (std::shared_ptr<JobQueue::Coro> const&)
                { unprotected = false; });
            BEAST_EXPECT (coro == nullptr);
        }
    }

public:
    void run() override
    {
        testAddJob();
        testPostCoro();
    }
};

BEAST_DEFINE_TESTSUITE(JobQueue, core, ripple);

} 
} 
























