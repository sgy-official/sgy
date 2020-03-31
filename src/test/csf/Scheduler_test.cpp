

#include <ripple/beast/unit_test.h>
#include <test/csf/Scheduler.h>
#include <set>

namespace ripple {
namespace test {

class Scheduler_test : public beast::unit_test::suite
{
public:

    void
    run() override
    {
        using namespace std::chrono_literals;
        csf::Scheduler scheduler;
        std::set<int> seen;

        scheduler.in(1s, [&]{ seen.insert(1);});
        scheduler.in(2s, [&]{ seen.insert(2);});
        auto token = scheduler.in(3s, [&]{ seen.insert(3);});
        scheduler.at(scheduler.now() + 4s, [&]{ seen.insert(4);});
        scheduler.at(scheduler.now() + 8s, [&]{ seen.insert(8);});

        auto start = scheduler.now();

        BEAST_EXPECT(seen.empty());
        BEAST_EXPECT(scheduler.step_one());
        BEAST_EXPECT(seen == std::set<int>({1}));
        BEAST_EXPECT(scheduler.now() == (start + 1s));

        BEAST_EXPECT(scheduler.step_until(scheduler.now()));
        BEAST_EXPECT(seen == std::set<int>({1}));
        BEAST_EXPECT(scheduler.now() == (start + 1s));

        BEAST_EXPECT(scheduler.step_for(1s));
        BEAST_EXPECT(seen == std::set<int>({1,2}));
        BEAST_EXPECT(scheduler.now() == (start + 2s));

        scheduler.cancel(token);
        BEAST_EXPECT(scheduler.step_for(1s));
        BEAST_EXPECT(seen == std::set<int>({1,2}));
        BEAST_EXPECT(scheduler.now() == (start + 3s));

        BEAST_EXPECT(scheduler.step_while([&]() { return seen.size() < 3; }));
        BEAST_EXPECT(seen == std::set<int>({1,2,4}));
        BEAST_EXPECT(scheduler.now() == (start + 4s));

        BEAST_EXPECT(scheduler.step());
        BEAST_EXPECT(seen == std::set<int>({1,2,4,8}));
        BEAST_EXPECT(scheduler.now() == (start + 8s));

        BEAST_EXPECT(!scheduler.step());
        BEAST_EXPECT(seen == std::set<int>({1,2,4,8}));
        BEAST_EXPECT(scheduler.now() == (start + 8s));
    }
};

BEAST_DEFINE_TESTSUITE(Scheduler, test, ripple);

}  
}  
























