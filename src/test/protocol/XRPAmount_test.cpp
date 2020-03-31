

#include <ripple/protocol/XRPAmount.h>
#include <ripple/beast/unit_test.h>

namespace ripple {

class XRPAmount_test : public beast::unit_test::suite
{
public:
    void testSigNum ()
    {
        testcase ("signum");

        for (auto i : { -1, 0, 1})
        {
            XRPAmount const x(i);

            if (i < 0)
                BEAST_EXPECT(x.signum () < 0);
            else if (i > 0)
                BEAST_EXPECT(x.signum () > 0);
            else
                BEAST_EXPECT(x.signum () == 0);
        }
    }

    void testBeastZero ()
    {
        testcase ("beast::Zero Comparisons");

        using beast::zero;

        for (auto i : { -1, 0, 1})
        {
            XRPAmount const x (i);

            BEAST_EXPECT((i == 0) == (x == zero));
            BEAST_EXPECT((i != 0) == (x != zero));
            BEAST_EXPECT((i < 0) == (x < zero));
            BEAST_EXPECT((i > 0) == (x > zero));
            BEAST_EXPECT((i <= 0) == (x <= zero));
            BEAST_EXPECT((i >= 0) == (x >= zero));

            BEAST_EXPECT((0 == i) == (zero == x));
            BEAST_EXPECT((0 != i) == (zero != x));
            BEAST_EXPECT((0 < i) == (zero < x));
            BEAST_EXPECT((0 > i) == (zero > x));
            BEAST_EXPECT((0 <= i) == (zero <= x));
            BEAST_EXPECT((0 >= i) == (zero >= x));
        }
    }

    void testComparisons ()
    {
        testcase ("XRP Comparisons");

        for (auto i : { -1, 0, 1})
        {
            XRPAmount const x (i);

            for (auto j : { -1, 0, 1})
            {
                XRPAmount const y (j);

                BEAST_EXPECT((i == j) == (x == y));
                BEAST_EXPECT((i != j) == (x != y));
                BEAST_EXPECT((i < j) == (x < y));
                BEAST_EXPECT((i > j) == (x > y));
                BEAST_EXPECT((i <= j) == (x <= y));
                BEAST_EXPECT((i >= j) == (x >= y));
            }
        }
    }

    void testAddSub ()
    {
        testcase ("Addition & Subtraction");

        for (auto i : { -1, 0, 1})
        {
            XRPAmount const x (i);

            for (auto j : { -1, 0, 1})
            {
                XRPAmount const y (j);

                BEAST_EXPECT(XRPAmount(i + j) == (x + y));
                BEAST_EXPECT(XRPAmount(i - j) == (x - y));

                BEAST_EXPECT((x + y) == (y + x));   
            }
        }
    }

    void testMulRatio()
    {
        testcase ("mulRatio");

        constexpr auto maxUInt32 = std::numeric_limits<std::uint32_t>::max ();
        constexpr auto maxUInt64 = std::numeric_limits<std::uint64_t>::max ();

        {
            XRPAmount big (maxUInt64);
            BEAST_EXPECT(big == mulRatio (big, maxUInt32, maxUInt32, true));
            BEAST_EXPECT(big == mulRatio (big, maxUInt32, maxUInt32, false));
        }

        {
            XRPAmount big (maxUInt64);
            BEAST_EXPECT(big == mulRatio (big, maxUInt32, maxUInt32, true));
            BEAST_EXPECT(big == mulRatio (big, maxUInt32, maxUInt32, false));
        }

        {
            XRPAmount tiny (1);
            BEAST_EXPECT(tiny == mulRatio (tiny, 1, maxUInt32, true));
            BEAST_EXPECT(beast::zero == mulRatio (tiny, 1, maxUInt32, false));
            BEAST_EXPECT(beast::zero ==
                mulRatio (tiny, maxUInt32 - 1, maxUInt32, false));

            XRPAmount tinyNeg (-1);
            BEAST_EXPECT(beast::zero == mulRatio (tinyNeg, 1, maxUInt32, true));
            BEAST_EXPECT(beast::zero == mulRatio (tinyNeg, maxUInt32 - 1, maxUInt32, true));
            BEAST_EXPECT(tinyNeg == mulRatio (tinyNeg, maxUInt32 - 1, maxUInt32, false));
        }

        {
            {
                XRPAmount one (1);
                auto const rup = mulRatio (one, maxUInt32 - 1, maxUInt32, true);
                auto const rdown = mulRatio (one, maxUInt32 - 1, maxUInt32, false);
                BEAST_EXPECT(rup.drops () - rdown.drops () == 1);
            }

            {
                XRPAmount big (maxUInt64);
                auto const rup = mulRatio (big, maxUInt32 - 1, maxUInt32, true);
                auto const rdown = mulRatio (big, maxUInt32 - 1, maxUInt32, false);
                BEAST_EXPECT(rup.drops () - rdown.drops () == 1);
            }

            {
                XRPAmount negOne (-1);
                auto const rup = mulRatio (negOne, maxUInt32 - 1, maxUInt32, true);
                auto const rdown = mulRatio (negOne, maxUInt32 - 1, maxUInt32, false);
                BEAST_EXPECT(rup.drops () - rdown.drops () == 1);
            }
        }

        {
            XRPAmount one (1);
            except ([&] {mulRatio (one, 1, 0, true);});
        }

        {
            XRPAmount big (maxUInt64);
            except ([&] {mulRatio (big, 2, 0, true);});
        }
    }


    void run () override
    {
        testSigNum ();
        testBeastZero ();
        testComparisons ();
        testAddSub ();
        testMulRatio ();
    }
};

BEAST_DEFINE_TESTSUITE(XRPAmount,protocol,ripple);

} 
























