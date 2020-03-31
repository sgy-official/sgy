

#include <ripple/protocol/Feature.h>
#include <ripple/protocol/jss.h>
#include <test/jtx.h>

namespace ripple {

class LedgerClosed_test : public beast::unit_test::suite
{
public:

    void testMonitorRoot()
    {
        using namespace test::jtx;
        Env env {*this, FeatureBitset{}};
        Account const alice {"alice"};
        env.fund(XRP(10000), alice);

        auto lc_result = env.rpc("ledger_closed") [jss::result];
        BEAST_EXPECT(lc_result[jss::ledger_hash]  == "8AEDBB96643962F1D40F01E25632ABB3C56C9F04B0231EE4B18248B90173D189");
        BEAST_EXPECT(lc_result[jss::ledger_index] == 2);

        env.close();
        auto const ar_master = env.le(env.master);
        BEAST_EXPECT(ar_master->getAccountID(sfAccount) == env.master.id());
        BEAST_EXPECT((*ar_master)[sfBalance] == drops( 99999989999999980 ));

        auto const ar_alice = env.le(alice);
        BEAST_EXPECT(ar_alice->getAccountID(sfAccount) == alice.id());
        BEAST_EXPECT((*ar_alice)[sfBalance] == XRP( 10000 ));

        lc_result = env.rpc("ledger_closed") [jss::result];
        BEAST_EXPECT(lc_result[jss::ledger_hash]  == "7C3EEDB3124D92E49E75D81A8826A2E65A75FD71FC3FD6F36FEB803C5F1D812D");
        BEAST_EXPECT(lc_result[jss::ledger_index] == 3);
    }

    void run() override
    {
        testMonitorRoot();
    }
};

BEAST_DEFINE_TESTSUITE(LedgerClosed,app,ripple);

}

























