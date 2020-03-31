

#include <ripple/basics/chrono.h>
#include <ripple/basics/TaggedCache.h>
#include <ripple/beast/unit_test.h>
#include <ripple/beast/clock/manual_clock.h>
#include <test/unit_test/SuiteJournal.h>

namespace ripple {



class TaggedCache_test : public beast::unit_test::suite
{
public:
    void run () override
    {
        using namespace std::chrono_literals;
        using namespace beast::severities;
        test::SuiteJournal journal ("TaggedCache_test", *this);

        TestStopwatch clock;
        clock.set (0);

        using Key = int;
        using Value = std::string;
        using Cache = TaggedCache <Key, Value>;

        Cache c ("test", 1, 1s, clock, journal);

        {
            BEAST_EXPECT(c.getCacheSize() == 0);
            BEAST_EXPECT(c.getTrackSize() == 0);
            BEAST_EXPECT(! c.insert (1, "one"));
            BEAST_EXPECT(c.getCacheSize() == 1);
            BEAST_EXPECT(c.getTrackSize() == 1);

            {
                std::string s;
                BEAST_EXPECT(c.retrieve (1, s));
                BEAST_EXPECT(s == "one");
            }

            ++clock;
            c.sweep ();
            BEAST_EXPECT(c.getCacheSize () == 0);
            BEAST_EXPECT(c.getTrackSize () == 0);
        }

        {
            BEAST_EXPECT(! c.insert (2, "two"));
            BEAST_EXPECT(c.getCacheSize() == 1);
            BEAST_EXPECT(c.getTrackSize() == 1);

            {
                Cache::mapped_ptr p (c.fetch (2));
                BEAST_EXPECT(p != nullptr);
                ++clock;
                c.sweep ();
                BEAST_EXPECT(c.getCacheSize() == 0);
                BEAST_EXPECT(c.getTrackSize() == 1);
            }

            ++clock;
            c.sweep ();
            BEAST_EXPECT(c.getCacheSize() == 0);
            BEAST_EXPECT(c.getTrackSize() == 0);
        }

        {
            BEAST_EXPECT(! c.insert (3, "three"));

            {
                Cache::mapped_ptr const p1 (c.fetch (3));
                Cache::mapped_ptr p2 (std::make_shared <Value> ("three"));
                c.canonicalize (3, p2);
                BEAST_EXPECT(p1.get() == p2.get());
            }
            ++clock;
            c.sweep ();
            BEAST_EXPECT(c.getCacheSize() == 0);
            BEAST_EXPECT(c.getTrackSize() == 0);
        }

        {
            BEAST_EXPECT(! c.insert (4, "four"));
            BEAST_EXPECT(c.getCacheSize() == 1);
            BEAST_EXPECT(c.getTrackSize() == 1);

            {
                Cache::mapped_ptr p1 (c.fetch (4));
                BEAST_EXPECT(p1 != nullptr);
                BEAST_EXPECT(c.getCacheSize() == 1);
                BEAST_EXPECT(c.getTrackSize() == 1);
                ++clock;
                c.sweep ();
                BEAST_EXPECT(c.getCacheSize() == 0);
                BEAST_EXPECT(c.getTrackSize() == 1);
                Cache::mapped_ptr p2 (std::make_shared <std::string> ("four"));
                BEAST_EXPECT(c.canonicalize (4, p2, false));
                BEAST_EXPECT(c.getCacheSize() == 1);
                BEAST_EXPECT(c.getTrackSize() == 1);
                BEAST_EXPECT(p1.get() == p2.get());
            }

            ++clock;
            c.sweep ();
            BEAST_EXPECT(c.getCacheSize() == 0);
            BEAST_EXPECT(c.getTrackSize() == 0);
        }
    }
};

BEAST_DEFINE_TESTSUITE(TaggedCache,common,ripple);

}
























