

#include <ripple/beast/core/CurrentThreadName.h>
#include <ripple/beast/unit_test.h>
#include <atomic>
#include <thread>

namespace ripple {
namespace test {

class CurrentThreadName_test : public beast::unit_test::suite
{
private:
    static void exerciseName (
        std::string myName, std::atomic<bool>* stop, std::atomic<int>* state)
    {
        auto const initialThreadName = beast::getCurrentThreadName();

        beast::setCurrentThreadName (myName);

        *state = 1;

        if (initialThreadName)
            return;

        while (! *stop);

        if (beast::getCurrentThreadName() == myName)
            *state = 2;
    }

public:
    void
    run() override
    {
        std::atomic<bool> stop {false};

        std::atomic<int> stateA {0};
        std::thread tA (exerciseName, "tA", &stop, &stateA);

        std::atomic<int> stateB {0};
        std::thread tB (exerciseName, "tB", &stop, &stateB);

        while (stateA == 0 || stateB == 0);

        stop = true;
        tA.join();
        tB.join();

        BEAST_EXPECT (stateA == 2);
        BEAST_EXPECT (stateB == 2);
    }
};

BEAST_DEFINE_TESTSUITE(CurrentThreadName,core,beast);

} 
} 
























