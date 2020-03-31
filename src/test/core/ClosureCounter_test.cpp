

#include <ripple/core/ClosureCounter.h>
#include <ripple/beast/unit_test.h>
#include <test/jtx/Env.h>
#include <atomic>
#include <chrono>
#include <thread>

namespace ripple {
namespace test {


class ClosureCounter_test : public beast::unit_test::suite
{
    jtx::Env env {*this};
    beast::Journal j {env.app().journal ("ClosureCounter_test")};

    void testConstruction()
    {
        {
            ClosureCounter<void> voidCounter;
            BEAST_EXPECT (voidCounter.count() == 0);

            int evidence = 0;
            auto wrapped = voidCounter.wrap ([&evidence] () { ++evidence; });
            BEAST_EXPECT (voidCounter.count() == 1);
            BEAST_EXPECT (evidence == 0);
            BEAST_EXPECT (wrapped);

            (*wrapped)();
            BEAST_EXPECT (evidence == 1);
            (*wrapped)();
            BEAST_EXPECT (evidence == 2);

            wrapped = boost::none;
            BEAST_EXPECT (voidCounter.count() == 0);
        }
        {
            ClosureCounter<void, int> setCounter;
            BEAST_EXPECT (setCounter.count() == 0);

            int evidence = 0;
            auto setInt = [&evidence] (int i) { evidence = i; };
            auto wrapped = setCounter.wrap (setInt);

            BEAST_EXPECT (setCounter.count() == 1);
            BEAST_EXPECT (evidence == 0);
            BEAST_EXPECT (wrapped);

            (*wrapped)(5);
            BEAST_EXPECT (evidence == 5);
            (*wrapped)(11);
            BEAST_EXPECT (evidence == 11);

            wrapped = boost::none;
            BEAST_EXPECT (setCounter.count() == 0);
        }
        {
            ClosureCounter<int, int, int> sumCounter;
            BEAST_EXPECT (sumCounter.count() == 0);

            auto const sum = [] (int i, int j) { return i + j; };
            auto wrapped = sumCounter.wrap (sum);

            BEAST_EXPECT (sumCounter.count() == 1);
            BEAST_EXPECT (wrapped);

            BEAST_EXPECT ((*wrapped)(5,  2) ==  7);
            BEAST_EXPECT ((*wrapped)(2, -8) == -6);

            wrapped = boost::none;
            BEAST_EXPECT (sumCounter.count() == 0);
        }
    }

    class TrackedString
    {
    public:
        int copies = {0};
        int moves = {0};
        std::string str;

        TrackedString() = delete;

        explicit TrackedString(char const* rhs)
        : str (rhs) {}

        TrackedString (TrackedString const& rhs)
        : copies (rhs.copies + 1)
        , moves (rhs.moves)
        , str (rhs.str) {}

        TrackedString (TrackedString&& rhs) noexcept
        : copies (rhs.copies)
        , moves (rhs.moves + 1)
        , str (std::move(rhs.str)) {}

        TrackedString& operator=(TrackedString const& rhs) = delete;

        TrackedString& operator+=(char const* rhs)
        {
            str += rhs;
            return *this;
        }

        friend
        TrackedString operator+(TrackedString const& str, char const* rhs)
        {
            TrackedString ret {str};
            ret.str += rhs;
            return ret;
        }
    };

    void testArgs()
    {
        {
            ClosureCounter<TrackedString, TrackedString> strCounter;
            BEAST_EXPECT (strCounter.count() == 0);

            auto wrapped = strCounter.wrap (
                [] (TrackedString in) { return in += "!"; });

            BEAST_EXPECT (strCounter.count() == 1);
            BEAST_EXPECT (wrapped);

            TrackedString const strValue ("value");
            TrackedString const result = (*wrapped)(strValue);
            BEAST_EXPECT (result.copies == 2);
            BEAST_EXPECT (result.moves == 1);
            BEAST_EXPECT (result.str == "value!");
            BEAST_EXPECT (strValue.str.size() == 5);
        }
        {
            ClosureCounter<TrackedString, TrackedString const&> strCounter;
            BEAST_EXPECT (strCounter.count() == 0);

            auto wrapped = strCounter.wrap (
                [] (TrackedString const& in) { return in + "!"; });

            BEAST_EXPECT (strCounter.count() == 1);
            BEAST_EXPECT (wrapped);

            TrackedString const strConstLValue ("const lvalue");
            TrackedString const result = (*wrapped)(strConstLValue);
            BEAST_EXPECT (result.copies == 1);
            BEAST_EXPECT (result.str == "const lvalue!");
            BEAST_EXPECT (strConstLValue.str.size() == 12);
        }
        {
            ClosureCounter<TrackedString, TrackedString&> strCounter;
            BEAST_EXPECT (strCounter.count() == 0);

            auto wrapped = strCounter.wrap (
                [] (TrackedString& in) { return in += "!"; });

            BEAST_EXPECT (strCounter.count() == 1);
            BEAST_EXPECT (wrapped);

            TrackedString strLValue ("lvalue");
            TrackedString const result = (*wrapped)(strLValue);
            BEAST_EXPECT (result.copies == 1);
            BEAST_EXPECT (result.moves == 0);
            BEAST_EXPECT (result.str == "lvalue!");
            BEAST_EXPECT (strLValue.str == result.str);
        }
        {
            ClosureCounter<TrackedString, TrackedString&&> strCounter;
            BEAST_EXPECT (strCounter.count() == 0);

            auto wrapped = strCounter.wrap (
                [] (TrackedString&& in) {
                    return std::move(in += "!");
                });

            BEAST_EXPECT (strCounter.count() == 1);
            BEAST_EXPECT (wrapped);

            TrackedString strRValue ("rvalue abcdefghijklmnopqrstuvwxyz");
            TrackedString const result = (*wrapped)(std::move(strRValue));
            BEAST_EXPECT (result.copies == 0);
            BEAST_EXPECT (result.moves == 1);
            BEAST_EXPECT (result.str == "rvalue abcdefghijklmnopqrstuvwxyz!");
            BEAST_EXPECT (strRValue.str.size() == 0);
        }
    }

    void testWrap()
    {
        ClosureCounter<void> voidCounter;
        BEAST_EXPECT (voidCounter.count() == 0);
        {
            auto wrapped1 = voidCounter.wrap ([] () {});
            BEAST_EXPECT (voidCounter.count() == 1);
            {
                auto wrapped2 (wrapped1);
                BEAST_EXPECT (voidCounter.count() == 2);
                {
                    auto wrapped3 (std::move(wrapped2));
                    BEAST_EXPECT (voidCounter.count() == 3);
                    {
                        auto wrapped4 = voidCounter.wrap ([] () {});
                        BEAST_EXPECT (voidCounter.count() == 4);
                    }
                    BEAST_EXPECT (voidCounter.count() == 3);
                }
                BEAST_EXPECT (voidCounter.count() == 2);
            }
            BEAST_EXPECT (voidCounter.count() == 1);
        }
        BEAST_EXPECT (voidCounter.count() == 0);

        using namespace std::chrono_literals;
        voidCounter.join("testWrap", 1ms, j);

        BEAST_EXPECT (voidCounter.wrap ([] () {}) == boost::none);
    }

    void testWaitOnJoin()
    {
        ClosureCounter<void> voidCounter;
        BEAST_EXPECT (voidCounter.count() == 0);

        auto wrapped = (voidCounter.wrap ([] () {}));
        BEAST_EXPECT (voidCounter.count() == 1);

        std::atomic<bool> threadExited {false};
        std::thread localThread ([&voidCounter, &threadExited, this] ()
        {
            using namespace std::chrono_literals;
            voidCounter.join("testWaitOnJoin", 1ms, j);
            threadExited.store (true);
        });

        while (! voidCounter.joined());

        using namespace std::chrono_literals;
        std::this_thread::sleep_for (5ms);
        BEAST_EXPECT (threadExited == false);

        wrapped = boost::none;
        BEAST_EXPECT (voidCounter.count() == 0);

        while (threadExited == false);
        localThread.join();
    }

public:
    void run() override
    {
        testConstruction();
        testArgs();
        testWrap();
        testWaitOnJoin();
    }
};

BEAST_DEFINE_TESTSUITE(ClosureCounter, core, ripple);

} 
} 
























