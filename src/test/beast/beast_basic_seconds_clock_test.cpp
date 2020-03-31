

#include <ripple/beast/unit_test.h>

#include <ripple/beast/clock/basic_seconds_clock.h>

namespace beast {

class basic_seconds_clock_test : public unit_test::suite
{
public:
    void
    run() override
    {
        basic_seconds_clock <
            std::chrono::steady_clock>::now ();
        pass ();
    }
};

BEAST_DEFINE_TESTSUITE(basic_seconds_clock,chrono,beast);

}
























