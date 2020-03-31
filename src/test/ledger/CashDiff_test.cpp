

#include <ripple/ledger/CashDiff.h>
#include <ripple/protocol/STAmount.h>
#include <ripple/beast/unit_test.h>
#include <type_traits>

namespace ripple {
namespace test {

class CashDiff_test : public beast::unit_test::suite
{
    static_assert(!std::is_default_constructible<CashDiff>{}, "");
    static_assert(!std::is_copy_constructible<CashDiff>{}, "");
    static_assert(!std::is_copy_assignable<CashDiff>{}, "");
    static_assert(std::is_nothrow_move_constructible<CashDiff>{}, "");
    static_assert(!std::is_move_assignable<CashDiff>{}, "");

    void
    testDust ()
    {
        testcase ("diffIsDust (STAmount, STAmount)");

        Issue const usd (Currency (0x5553440000000000), AccountID (0x4985601));
        Issue const usf (Currency (0x5553460000000000), AccountID (0x4985601));

        expect (!diffIsDust (STAmount{usd, 1}, STAmount{usd, -1}));

        expect (!diffIsDust (STAmount{usd, 1}, STAmount{usf, 1}));

        expect (!diffIsDust (STAmount{usd, 1}, STAmount{1}));

        expect (diffIsDust (STAmount{0}, STAmount{0}));
        {
            std::uint64_t oldProbe = 0;
            std::uint64_t newProbe = 10;
            std::uint8_t e10 = 1;
            do
            {
                STAmount large (usd, newProbe + 1);
                STAmount small (usd, newProbe);

                expect (diffIsDust (large, small, e10));
                expect (diffIsDust (large, small, e10+1) == (e10 > 13));

                oldProbe = newProbe;
                newProbe = oldProbe * 10;
                e10 += 1;
            } while (newProbe > oldProbe);
        }
        {
            expect (diffIsDust (STAmount{2}, STAmount{0}));

            std::uint64_t oldProbe = 0;
            std::uint64_t newProbe = 10;
            std::uint8_t e10 = 0;
            do
            {
                STAmount large (newProbe + 3);
                STAmount small (newProbe);

                expect (diffIsDust (large, small, e10));
                expect (diffIsDust (large, small, e10+1) == (e10 >= 20));

                oldProbe = newProbe;
                newProbe = oldProbe * 10;
                e10 += 1;
            } while (newProbe > oldProbe);
        }
    }

public:
    void run () override
    {
        testDust();
    }
};

BEAST_DEFINE_TESTSUITE (CashDiff, ledger, ripple);

}  
}  
























