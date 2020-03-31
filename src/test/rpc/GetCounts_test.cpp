

#include <test/jtx.h>
#include <ripple/beast/unit_test.h>
#include <ripple/protocol/jss.h>
#include <ripple/protocol/SField.h>
#include <ripple/basics/CountedObject.h>

namespace ripple {

class GetCounts_test : public beast::unit_test::suite
{
    void testGetCounts()
    {
        using namespace test::jtx;
        Env env(*this);

        Json::Value result;
        {
            result = env.rpc("get_counts")[jss::result];
            BEAST_EXPECT(result[jss::status] == "success");
            BEAST_EXPECT(! result.isMember("Transaction"));
            BEAST_EXPECT(! result.isMember("STObject"));
            BEAST_EXPECT(! result.isMember("HashRouterEntry"));
            BEAST_EXPECT(
                result.isMember(jss::uptime) &&
                ! result[jss::uptime].asString().empty());
            BEAST_EXPECT(
                result.isMember(jss::dbKBTotal) &&
                result[jss::dbKBTotal].asInt() > 0);
        }

        env.close();
        Account alice {"alice"};
        Account bob {"bob"};
        env.fund (XRP(10000), alice, bob);
        env.trust (alice["USD"](1000), bob);
        for(auto i=0; i<20; ++i)
        {
            env (pay (alice, bob, alice["USD"](5)));
            env.close();
        }

        {
            result = env.rpc("get_counts")[jss::result];
            BEAST_EXPECT(result[jss::status] == "success");
            auto const& objectCounts = CountedObjects::getInstance ().getCounts (10);
            for (auto const& it : objectCounts)
            {
                BEAST_EXPECTS(result.isMember(it.first), it.first);
                BEAST_EXPECTS(result[it.first].asInt() == it.second, it.first);
            }
            BEAST_EXPECT(! result.isMember(jss::local_txs));
        }

        {
            result = env.rpc("get_counts", "100")[jss::result];
            BEAST_EXPECT(result[jss::status] == "success");

            auto const& objectCounts = CountedObjects::getInstance ().getCounts (100);
            for (auto const& it : objectCounts)
            {
                BEAST_EXPECTS(result.isMember(it.first), it.first);
                BEAST_EXPECTS(result[it.first].asInt() == it.second, it.first);
            }
            BEAST_EXPECT(! result.isMember("Transaction"));
            BEAST_EXPECT(! result.isMember("STTx"));
            BEAST_EXPECT(! result.isMember("STArray"));
            BEAST_EXPECT(! result.isMember("HashRouterEntry"));
            BEAST_EXPECT(! result.isMember("STLedgerEntry"));
        }

        {
            env (pay (alice, bob, alice["USD"](5)));
            result = env.rpc("get_counts")[jss::result];
            BEAST_EXPECT(
                result.isMember(jss::local_txs) &&
                result[jss::local_txs].asInt() > 0);
        }
    }

public:
    void run () override
    {
        testGetCounts();
    }
};

BEAST_DEFINE_TESTSUITE(GetCounts,rpc,ripple);

} 

























