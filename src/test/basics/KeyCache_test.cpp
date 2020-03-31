

#include <ripple/basics/chrono.h>
#include <ripple/basics/KeyCache.h>
#include <ripple/beast/unit_test.h>
#include <ripple/beast/clock/manual_clock.h>

namespace ripple {

class KeyCache_test : public beast::unit_test::suite
{
public:
    void run () override
    {
        using namespace std::chrono_literals;
        TestStopwatch clock;
        clock.set (0);

        using Key = std::string;
        using Cache = KeyCache <Key>;

        {
            Cache c ("test", clock, 1, 2s);

            BEAST_EXPECT(c.size () == 0);
            BEAST_EXPECT(c.insert ("one"));
            BEAST_EXPECT(! c.insert ("one"));
            BEAST_EXPECT(c.size () == 1);
            BEAST_EXPECT(c.exists ("one"));
            BEAST_EXPECT(c.touch_if_exists ("one"));
            ++clock;
            c.sweep ();
            BEAST_EXPECT(c.size () == 1);
            BEAST_EXPECT(c.exists ("one"));
            ++clock;
            c.sweep ();
            BEAST_EXPECT(c.size () == 0);
            BEAST_EXPECT(! c.exists ("one"));
            BEAST_EXPECT(! c.touch_if_exists ("one"));
        }

        {
            Cache c ("test", clock, 2, 2s);

            BEAST_EXPECT(c.insert ("one"));
            BEAST_EXPECT(c.size  () == 1);
            BEAST_EXPECT(c.insert ("two"));
            BEAST_EXPECT(c.size  () == 2);
            ++clock;
            c.sweep ();
            BEAST_EXPECT(c.size () == 2);
            BEAST_EXPECT(c.touch_if_exists ("two"));
            ++clock;
            c.sweep ();
            BEAST_EXPECT(c.size () == 1);
            BEAST_EXPECT(c.exists ("two"));
        }

        {
            Cache c ("test", clock, 2, 3s);

            BEAST_EXPECT(c.insert ("one"));
            ++clock;
            BEAST_EXPECT(c.insert ("two"));
            ++clock;
            BEAST_EXPECT(c.insert ("three"));
            ++clock;
            BEAST_EXPECT(c.size () == 3);
            c.sweep ();
            BEAST_EXPECT(c.size () < 3);
        }
    }
};

BEAST_DEFINE_TESTSUITE(KeyCache,common,ripple);

}
























