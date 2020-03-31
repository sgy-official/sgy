

#include <ripple/basics/RangeSet.h>
#include <ripple/beast/unit_test.h>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

namespace ripple
{
class RangeSet_test : public beast::unit_test::suite
{
public:
    void
    testPrevMissing()
    {
        testcase("prevMissing");


        RangeSet<std::uint32_t> set;
        for (std::uint32_t i = 0; i < 10; ++i)
            set.insert(range(10 * i, 10 * i + 5));

        for (std::uint32_t i = 1; i < 100; ++i)
        {
            boost::optional<std::uint32_t> expected;
            if (i > 6)
            {
                std::uint32_t const oneBelowRange = (10 * (i / 10)) - 1;

                expected = ((i % 10) > 6) ? (i - 1) : oneBelowRange;
            }
            BEAST_EXPECT(prevMissing(set, i) == expected);
        }
    }

    void
    testToString()
    {
        testcase("toString");

        RangeSet<std::uint32_t> set;
        BEAST_EXPECT(to_string(set) == "empty");

        set.insert(1);
        BEAST_EXPECT(to_string(set) == "1");

        set.insert(range(4u, 6u));
        BEAST_EXPECT(to_string(set) == "1,4-6");

        set.insert(2);
        BEAST_EXPECT(to_string(set) == "1-2,4-6");

        set.erase(range(4u, 5u));
        BEAST_EXPECT(to_string(set) == "1-2,6");
    }

    void
    testSerialization()
    {

        auto works = [](RangeSet<std::uint32_t> const & orig)
        {
            std::stringstream ss;
            boost::archive::binary_oarchive oa(ss);
            oa << orig;

            boost::archive::binary_iarchive ia(ss);
            RangeSet<std::uint32_t> deser;
            ia >> deser;

            return orig == deser;
        };

        RangeSet<std::uint32_t> rs;

        BEAST_EXPECT(works(rs));

        rs.insert(3);
        BEAST_EXPECT(works(rs));

        rs.insert(range(7u, 10u));
        BEAST_EXPECT(works(rs));

    }
    void
    run() override
    {
        testPrevMissing();
        testToString();
        testSerialization();
    }
};

BEAST_DEFINE_TESTSUITE(RangeSet, ripple_basics, ripple);

}  
























