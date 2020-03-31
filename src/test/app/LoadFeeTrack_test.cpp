

#include <ripple/app/misc/LoadFeeTrack.h>
#include <ripple/core/Config.h>
#include <ripple/beast/unit_test.h>
#include <ripple/ledger/ReadView.h>

namespace ripple {

class LoadFeeTrack_test : public beast::unit_test::suite
{
public:
    void run () override
    {
        Config d; 
        LoadFeeTrack l;
        Fees const fees = [&]()
        {
            Fees f;
            f.base = d.FEE_DEFAULT;
            f.units = d.TRANSACTION_FEE_BASE;
            f.reserve = 200 * SYSTEM_CURRENCY_PARTS;
            f.increment = 50 * SYSTEM_CURRENCY_PARTS;
            return f;
        }();

        BEAST_EXPECT (scaleFeeLoad (10000, l, fees, false) == 10000);
        BEAST_EXPECT (scaleFeeLoad (1, l, fees, false) == 1);
    }
};

BEAST_DEFINE_TESTSUITE(LoadFeeTrack,ripple_core,ripple);

} 
























