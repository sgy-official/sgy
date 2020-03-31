

#include <ripple/core/ConfigSections.h>
#include <ripple/protocol/jss.h>
#include <test/jtx/WSClient.h>
#include <test/jtx/JSONRPCClient.h>
#include <test/jtx.h>
#include <ripple/beast/unit_test.h>

namespace ripple {
namespace test {

class RPCOverload_test : public beast::unit_test::suite
{
public:
    void testOverload(bool useWS)
    {
        testcase << "Overload " << (useWS ? "WS" : "HTTP") << " RPC client";
        using namespace jtx;
        Env env {*this, envconfig([](std::unique_ptr<Config> cfg)
            {
                cfg->loadFromString ("[" SECTION_SIGNING_SUPPORT "]\ntrue");
                return no_admin(std::move(cfg));
            })};

        Account const alice {"alice"};
        Account const bob {"bob"};
        env.fund (XRP (10000), alice, bob);

        std::unique_ptr<AbstractClient> client = useWS ?
              makeWSClient(env.app().config())
            : makeJSONRPCClient(env.app().config());

        Json::Value tx = Json::objectValue;
        tx[jss::tx_json] = pay(alice, bob, XRP(1));
        tx[jss::secret] = toBase58(generateSeed("alice"));

        bool warned = false, booted = false;
        for(int i = 0 ; i < 500 && !booted; ++i)
        {
            auto jv = client->invoke("sign", tx);
            if(!useWS)
                jv = jv[jss::result];
            if(jv.isNull())
                booted = true;
            else if (!(jv.isMember(jss::status) &&
                       (jv[jss::status] == "success")))
            {
                fail("", __FILE__, __LINE__);
            }

            if(jv.isMember(jss::warning))
                warned = jv[jss::warning] == jss::load;
        }
        BEAST_EXPECT(warned && booted);
    }



    void run() override
    {
        testOverload(false );
        testOverload(true );
    }
};

BEAST_DEFINE_TESTSUITE(RPCOverload,app,ripple);

} 
} 
























