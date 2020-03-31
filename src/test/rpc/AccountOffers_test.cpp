

#include <ripple/protocol/jss.h>
#include <test/jtx.h>

namespace ripple {
namespace test {

class AccountOffers_test : public beast::unit_test::suite
{
public:
    static bool checkArraySize(Json::Value const& val, unsigned int size)
    {
        return val.isArray() &&
               val.size() == size;
    }

    static bool checkMarker(Json::Value const& val)
    {
        return val.isMember(jss::marker) &&
               val[jss::marker].isString() &&
               val[jss::marker].asString().size() > 0;
    }

    void testNonAdminMinLimit()
    {
        using namespace jtx;
        Env env {*this, envconfig(no_admin)};
        Account const gw ("G1");
        auto const USD_gw  = gw["USD"];
        Account const bob ("bob");
        auto const USD_bob = bob["USD"];

        env.fund(XRP(10000), gw, bob);
        env.trust(USD_gw(1000), bob);

        env (pay (gw, bob, USD_gw (10)));
        unsigned const offer_count = 12u;
        for (auto i = 0u; i < offer_count; i++)
        {
            Json::Value jvo = offer(bob, XRP(100 + i), USD_gw(1));
            jvo[sfExpiration.fieldName] = 10000000u;
            env(jvo);
        }

        auto const jro_nl = env.rpc("account_offers", bob.human())[jss::result][jss::offers];
        BEAST_EXPECT(checkArraySize(jro_nl, offer_count));

        Json::Value jvParams;
        jvParams[jss::account] = bob.human();
        jvParams[jss::limit]   = 1u;
        auto const  jrr_l = env.rpc ("json", "account_offers", jvParams.toStyledString())[jss::result];
        auto const& jro_l = jrr_l[jss::offers];
        BEAST_EXPECT(checkMarker(jrr_l));
        BEAST_EXPECT(checkArraySize(jro_l, 10u));
    }

    void testSequential(bool asAdmin)
    {
        using namespace jtx;
        Env env {*this, asAdmin ? envconfig() : envconfig(no_admin)};
        Account const gw ("G1");
        auto const USD_gw  = gw["USD"];
        Account const bob ("bob");
        auto const USD_bob = bob["USD"];

        env.fund(XRP(10000), gw, bob);
        env.trust(USD_gw(1000), bob);

        env (pay (gw, bob, USD_gw (10)));

        env(offer(bob, XRP(100), USD_bob(1)));
        env(offer(bob, XRP(100), USD_gw(1)));
        env(offer(bob, XRP(10),  USD_gw(2)));

        auto const jro = env.rpc("account_offers", bob.human())[jss::result][jss::offers];
        if(BEAST_EXPECT(checkArraySize(jro, 3u)))
        {
            BEAST_EXPECT(jro[0u][jss::quality]                   == "100000000");
            BEAST_EXPECT(jro[0u][jss::taker_gets][jss::currency] == "USD");
            BEAST_EXPECT(jro[0u][jss::taker_gets][jss::issuer]   == gw.human());
            BEAST_EXPECT(jro[0u][jss::taker_gets][jss::value]    == "1");
            BEAST_EXPECT(jro[0u][jss::taker_pays]                == "100000000");

            BEAST_EXPECT(jro[1u][jss::quality]                   == "5000000");
            BEAST_EXPECT(jro[1u][jss::taker_gets][jss::currency] == "USD");
            BEAST_EXPECT(jro[1u][jss::taker_gets][jss::issuer]   == gw.human());
            BEAST_EXPECT(jro[1u][jss::taker_gets][jss::value]    == "2");
            BEAST_EXPECT(jro[1u][jss::taker_pays]                == "10000000");

            BEAST_EXPECT(jro[2u][jss::quality]                   == "100000000");
            BEAST_EXPECT(jro[2u][jss::taker_gets][jss::currency] == "USD");
            BEAST_EXPECT(jro[2u][jss::taker_gets][jss::issuer]   == bob.human());
            BEAST_EXPECT(jro[2u][jss::taker_gets][jss::value]    == "1");
            BEAST_EXPECT(jro[2u][jss::taker_pays]                == "100000000");
        }

        {
            Json::Value jvParams;
            jvParams[jss::account] = bob.human();
            jvParams[jss::limit]   = 1u;
            auto const  jrr_l_1 = env.rpc ("json", "account_offers", jvParams.toStyledString())[jss::result];
            auto const& jro_l_1 = jrr_l_1[jss::offers];
            BEAST_EXPECT(checkArraySize(jro_l_1, asAdmin ? 1u : 3u));
            BEAST_EXPECT(asAdmin ? checkMarker(jrr_l_1) : (! jrr_l_1.isMember(jss::marker)));
            if (asAdmin)
            {
                BEAST_EXPECT(jro[0u] == jro_l_1[0u]);

                jvParams[jss::marker] = jrr_l_1[jss::marker];
                auto const  jrr_l_2 = env.rpc ("json", "account_offers", jvParams.toStyledString())[jss::result];
                auto const& jro_l_2 = jrr_l_2[jss::offers];
                BEAST_EXPECT(checkMarker(jrr_l_2));
                BEAST_EXPECT(checkArraySize(jro_l_2, 1u));
                BEAST_EXPECT(jro[1u] == jro_l_2[0u]);

                jvParams[jss::marker] = jrr_l_2[jss::marker];
                auto const  jrr_l_3 = env.rpc ("json", "account_offers", jvParams.toStyledString())[jss::result];
                auto const& jro_l_3 = jrr_l_3[jss::offers];
                BEAST_EXPECT(! jrr_l_3.isMember(jss::marker));
                BEAST_EXPECT(checkArraySize(jro_l_3, 1u));
                BEAST_EXPECT(jro[2u] == jro_l_3[0u]);
            }
            else
            {
                BEAST_EXPECT(jro == jro_l_1);
            }
        }

        {
            Json::Value jvParams;
            jvParams[jss::account] = bob.human();
            jvParams[jss::limit]   = 0u;
            auto const  jrr = env.rpc ("json", "account_offers", jvParams.toStyledString())[jss::result];
            auto const& jro = jrr[jss::offers];
            BEAST_EXPECT(checkArraySize(jro, asAdmin ? 0u : 3u));
            BEAST_EXPECT(asAdmin ? checkMarker(jrr) : (! jrr.isMember(jss::marker)));
        }

    }

    void testBadInput()
    {
        using namespace jtx;
        Env env(*this);
        Account const gw ("G1");
        auto const USD_gw  = gw["USD"];
        Account const bob ("bob");
        auto const USD_bob = bob["USD"];

        env.fund(XRP(10000), gw, bob);
        env.trust(USD_gw(1000), bob);

        {
            auto const jrr = env.rpc ("account_offers");
            BEAST_EXPECT(jrr[jss::error]         == "badSyntax");
            BEAST_EXPECT(jrr[jss::status]        == "error");
            BEAST_EXPECT(jrr[jss::error_message] == "Syntax error.");
        }

        {
            Json::Value jvParams;
            jvParams[jss::account] = "";
            auto const jrr = env.rpc ("json", "account_offers", jvParams.toStyledString())[jss::result];
            BEAST_EXPECT(jrr[jss::error]         == "badSeed");
            BEAST_EXPECT(jrr[jss::status]        == "error");
            BEAST_EXPECT(jrr[jss::error_message] == "Disallowed seed.");
        }

        {
            auto const jrr = env.rpc ("account_offers", "rNOT_AN_ACCOUNT")[jss::result];
            BEAST_EXPECT(jrr[jss::error]         == "actNotFound");
            BEAST_EXPECT(jrr[jss::status]        == "error");
            BEAST_EXPECT(jrr[jss::error_message] == "Account not found.");
        }

        {
            Json::Value jvParams;
            jvParams[jss::account] = bob.human();
            jvParams[jss::limit]   = "0"; 
            auto const jrr = env.rpc ("json", "account_offers", jvParams.toStyledString())[jss::result];
            BEAST_EXPECT(jrr[jss::error]         == "invalidParams");
            BEAST_EXPECT(jrr[jss::status]        == "error");
            BEAST_EXPECT(jrr[jss::error_message] == "Invalid field 'limit', not unsigned integer.");
        }

        {
            Json::Value jvParams;
            jvParams[jss::account] = bob.human();
            jvParams[jss::marker]   = "NOT_A_MARKER";
            auto const jrr = env.rpc ("json", "account_offers", jvParams.toStyledString())[jss::result];
            BEAST_EXPECT(jrr[jss::error]         == "invalidParams");
            BEAST_EXPECT(jrr[jss::status]        == "error");
            BEAST_EXPECT(jrr[jss::error_message] == "Invalid parameters.");
        }

        {
            Json::Value jvParams;
            jvParams[jss::account] = bob.human();
            jvParams[jss::marker]   = 1;
            auto const jrr = env.rpc ("json", "account_offers", jvParams.toStyledString())[jss::result];
            BEAST_EXPECT(jrr[jss::error]         == "invalidParams");
            BEAST_EXPECT(jrr[jss::status]        == "error");
            BEAST_EXPECT(jrr[jss::error_message] == "Invalid field 'marker', not string.");
        }

        {
            Json::Value jvParams;
            jvParams[jss::account] = bob.human();
            jvParams[jss::ledger_index]   = 10u;
            auto const jrr = env.rpc ("json", "account_offers", jvParams.toStyledString())[jss::result];
            BEAST_EXPECT(jrr[jss::error]         == "lgrNotFound");
            BEAST_EXPECT(jrr[jss::status]        == "error");
            BEAST_EXPECT(jrr[jss::error_message] == "ledgerNotFound");
        }

    }

    void run() override
    {
        testSequential(true);
        testSequential(false);
        testBadInput();
        testNonAdminMinLimit();
    }

};

BEAST_DEFINE_TESTSUITE(AccountOffers,app,ripple);

}
}

























