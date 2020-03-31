
#include <test/jtx.h>
#include <ripple/protocol/AccountID.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/jss.h>
#include <ripple/protocol/SField.h>
#include <ripple/protocol/TxFlags.h>

namespace ripple {

class Freeze_test : public beast::unit_test::suite
{

    static Json::Value
    getAccountLines(test::jtx::Env& env, test::jtx::Account const& account)
    {
        Json::Value jq;
        jq[jss::account] = account.human();
        return env.rpc("json", "account_lines", to_string(jq))[jss::result];
    }

    static Json::Value
    getAccountOffers(
        test::jtx::Env& env,
        test::jtx::Account const& account,
        bool current = false)
    {
        Json::Value jq;
        jq[jss::account] = account.human();
        jq[jss::ledger_index] = current ? "current" : "validated";
        return env.rpc("json", "account_offers", to_string(jq))[jss::result];
    }

    static bool checkArraySize(Json::Value const& val, unsigned int size)
    {
        return val.isArray() && val.size() == size;
    }

    void testRippleState(FeatureBitset features)
    {
        testcase("RippleState Freeze");

        using namespace test::jtx;
        Env env(*this, features);

        Account G1 {"G1"};
        Account alice {"alice"};
        Account bob {"bob"};

        env.fund(XRP(1000), G1, alice, bob);
        env.close();

        env.trust(G1["USD"](100), bob);
        env.trust(G1["USD"](100), alice);
        env.close();

        env(pay(G1, bob, G1["USD"](10)));
        env(pay(G1, alice, G1["USD"](100)));
        env.close();

        env(offer(alice, XRP(500), G1["USD"](100)));
        env.close();

        {
            auto lines = getAccountLines(env, bob);
            if(! BEAST_EXPECT(checkArraySize(lines[jss::lines], 1u)))
                return;
            BEAST_EXPECT(lines[jss::lines][0u][jss::account] == G1.human());
            BEAST_EXPECT(lines[jss::lines][0u][jss::limit] == "100");
            BEAST_EXPECT(lines[jss::lines][0u][jss::balance] == "10");
        }

        {
            auto lines = getAccountLines(env, alice);
            if(! BEAST_EXPECT(checkArraySize(lines[jss::lines], 1u)))
                return;
            BEAST_EXPECT(lines[jss::lines][0u][jss::account] == G1.human());
            BEAST_EXPECT(lines[jss::lines][0u][jss::limit] == "100");
            BEAST_EXPECT(lines[jss::lines][0u][jss::balance] == "100");
        }

        {
            env(pay(alice, bob, G1["USD"](1)));

            env(pay(bob, alice, G1["USD"](1)));
            env.close();
        }

        {
            env(trust(G1, bob["USD"](0), tfSetFreeze));
            auto affected = env.meta()->
                getJson(JsonOptions::none)[sfAffectedNodes.fieldName];
            if(! BEAST_EXPECT(checkArraySize(affected, 2u)))
                return;
            auto ff =
                affected[1u][sfModifiedNode.fieldName][sfFinalFields.fieldName];
            BEAST_EXPECT(
                ff[sfLowLimit.fieldName] ==
                G1["USD"](0).value().getJson(JsonOptions::none));
            BEAST_EXPECT(ff[jss::Flags].asUInt() & lsfLowFreeze);
            BEAST_EXPECT(! (ff[jss::Flags].asUInt() & lsfHighFreeze));
            env.close();
        }

        {
            env(offer(bob, G1["USD"](5), XRP(25)));
            auto affected = env.meta()->
                getJson(JsonOptions::none)[sfAffectedNodes.fieldName];
            if(! BEAST_EXPECT(checkArraySize(affected, 5u)))
                return;
            auto ff =
                affected[3u][sfModifiedNode.fieldName][sfFinalFields.fieldName];
            BEAST_EXPECT(
                ff[sfHighLimit.fieldName] ==
                bob["USD"](100).value().getJson(JsonOptions::none));
            auto amt =
                STAmount{Issue{to_currency("USD"), noAccount()}, -15}
                    .value().getJson(JsonOptions::none);
            BEAST_EXPECT(ff[sfBalance.fieldName] == amt);
            env.close();
        }

        {
            env(offer(bob, XRP(1), G1["USD"](5)),
                ter(tecUNFUNDED_OFFER));

            env(pay(alice, bob, G1["USD"](1)));

            env(pay(bob, alice, G1["USD"](1)), ter(tecPATH_DRY));
        }

        {
            auto lines = getAccountLines(env, G1);
            Json::Value bobLine;
            for (auto const& it : lines[jss::lines])
            {
                if(it[jss::account] == bob.human())
                {
                    bobLine = it;
                    break;
                }
            }
            if(! BEAST_EXPECT(bobLine))
                return;
            BEAST_EXPECT(bobLine[jss::freeze] == true);
            BEAST_EXPECT(bobLine[jss::balance] == "-16");
        }

        {
            auto lines = getAccountLines(env, bob);
            Json::Value g1Line;
            for (auto const& it : lines[jss::lines])
            {
                if(it[jss::account] == G1.human())
                {
                    g1Line = it;
                    break;
                }
            }
            if(! BEAST_EXPECT(g1Line))
                return;
            BEAST_EXPECT(g1Line[jss::freeze_peer] == true);
            BEAST_EXPECT(g1Line[jss::balance] == "16");
        }

        {
            env(trust(G1, bob["USD"](0), tfClearFreeze));
            auto affected = env.meta()->
                getJson(JsonOptions::none)[sfAffectedNodes.fieldName];
            if(! BEAST_EXPECT(checkArraySize(affected, 2u)))
                return;
            auto ff =
                affected[1u][sfModifiedNode.fieldName][sfFinalFields.fieldName];
            BEAST_EXPECT(
                ff[sfLowLimit.fieldName] ==
                G1["USD"](0).value().getJson(JsonOptions::none));
            BEAST_EXPECT(! (ff[jss::Flags].asUInt() & lsfLowFreeze));
            BEAST_EXPECT(! (ff[jss::Flags].asUInt() & lsfHighFreeze));
            env.close();
        }
    }

    void
    testGlobalFreeze(FeatureBitset features)
    {
        testcase("Global Freeze");

        using namespace test::jtx;
        Env env(*this, features);

        Account G1 {"G1"};
        Account A1 {"A1"};
        Account A2 {"A2"};
        Account A3 {"A3"};
        Account A4 {"A4"};

        env.fund(XRP(12000), G1);
        env.fund(XRP(1000), A1);
        env.fund(XRP(20000), A2, A3, A4);
        env.close();

        env.trust(G1["USD"](1200), A1);
        env.trust(G1["USD"](200), A2);
        env.trust(G1["BTC"](100), A3);
        env.trust(G1["BTC"](100), A4);
        env.close();

        env(pay(G1, A1, G1["USD"](1000)));
        env(pay(G1, A2, G1["USD"](100)));
        env(pay(G1, A3, G1["BTC"](100)));
        env(pay(G1, A4, G1["BTC"](100)));
        env.close();

        env(offer(G1, XRP(10000), G1["USD"](100)), txflags(tfPassive));
        env(offer(G1, G1["USD"](100), XRP(10000)), txflags(tfPassive));
        env(offer(A1, XRP(10000), G1["USD"](100)), txflags(tfPassive));
        env(offer(A2, G1["USD"](100), XRP(10000)), txflags(tfPassive));
        env.close();

        {
            env.require(nflags(G1, asfGlobalFreeze));
            env(fset(G1, asfGlobalFreeze));
            env.require(flags(G1, asfGlobalFreeze));
            env.require(nflags(G1, asfNoFreeze));

            env(fclear(G1, asfGlobalFreeze));
            env.require(nflags(G1, asfGlobalFreeze));
            env.require(nflags(G1, asfNoFreeze));
        }

        {
            auto offers =
                env.rpc("book_offers",
                    std::string("USD/")+G1.human(), "XRP")
                [jss::result][jss::offers];
            if(! BEAST_EXPECT(checkArraySize(offers, 2u)))
                return;
            std::set<std::string> accounts;
            for (auto const& offer : offers)
            {
                accounts.insert(offer[jss::Account].asString());
            }
            BEAST_EXPECT(accounts.find(A2.human()) != std::end(accounts));
            BEAST_EXPECT(accounts.find(G1.human()) != std::end(accounts));

            offers =
                env.rpc("book_offers",
                    "XRP", std::string("USD/")+G1.human())
                [jss::result][jss::offers];
            if(! BEAST_EXPECT(checkArraySize(offers, 2u)))
                return;
            accounts.clear();
            for (auto const& offer : offers)
            {
                accounts.insert(offer[jss::Account].asString());
            }
            BEAST_EXPECT(accounts.find(A1.human()) != std::end(accounts));
            BEAST_EXPECT(accounts.find(G1.human()) != std::end(accounts));
        }

        {
            env(offer(A3, G1["BTC"](1), XRP(1)));

            env(offer(A4, XRP(1), G1["BTC"](1)));

            env(pay(G1, A2, G1["USD"](1)));

            env(pay(A2, G1, G1["USD"](1)));

            env(pay(A2, A1, G1["USD"](1)));

            env(pay(A1, A2, G1["USD"](1)));
        }

        {
            env.require(nflags(G1, asfGlobalFreeze));
            env(fset(G1, asfGlobalFreeze));
            env.require(flags(G1, asfGlobalFreeze));
            env.require(nflags(G1, asfNoFreeze));

            env(offer(A3, G1["BTC"](1), XRP(1)), ter(tecFROZEN));

            env(offer(A4, XRP(1), G1["BTC"](1)), ter(tecFROZEN));
        }

        {
            auto offers = getAccountOffers(env, G1)[jss::offers];
            if(! BEAST_EXPECT(checkArraySize(offers, 2u)))
                return;

            offers =
                env.rpc("book_offers",
                    "XRP", std::string("USD/")+G1.human())
                [jss::result][jss::offers];
            if(! BEAST_EXPECT(checkArraySize(offers, 2u)))
                return;

            offers =
                env.rpc("book_offers",
                    std::string("USD/")+G1.human(), "XRP")
                [jss::result][jss::offers];
            if(! BEAST_EXPECT(checkArraySize(offers, 2u)))
                return;
        }

        {
            env(pay(G1, A2, G1["USD"](1)));

            env(pay(A2, G1, G1["USD"](1)));

            env(pay(A2, A1, G1["USD"](1)), ter(tecPATH_DRY));
        }
    }

    void
    testNoFreeze(FeatureBitset features)
    {
        testcase("No Freeze");

        using namespace test::jtx;
        Env env(*this, features);

        Account G1 {"G1"};
        Account A1 {"A1"};

        env.fund(XRP(12000), G1);
        env.fund(XRP(1000), A1);
        env.close();

        env.trust(G1["USD"](1000), A1);
        env.close();

        env(pay(G1, A1, G1["USD"](1000)));
        env.close();

        env.require(nflags(G1, asfNoFreeze));
        env(fset(G1, asfNoFreeze));
        env.require(flags(G1, asfNoFreeze));
        env.require(nflags(G1, asfGlobalFreeze));

        env(fclear(G1, asfNoFreeze));
        env.require(flags(G1, asfNoFreeze));
        env.require(nflags(G1, asfGlobalFreeze));

        env(fset(G1, asfGlobalFreeze));
        env.require(flags(G1, asfNoFreeze));
        env.require(flags(G1, asfGlobalFreeze));

        env(fclear(G1, asfGlobalFreeze));
        env.require(flags(G1, asfNoFreeze));
        env.require(flags(G1, asfGlobalFreeze));

        env(trust(G1, A1["USD"](0), tfSetFreeze));
        auto affected =
            env.meta()->getJson(JsonOptions::none)[sfAffectedNodes.fieldName];
        if(! BEAST_EXPECT(checkArraySize(affected, 1u)))
            return;

        auto let =
            affected[0u][sfModifiedNode.fieldName][sfLedgerEntryType.fieldName];
        BEAST_EXPECT(let == jss::AccountRoot);
    }

    void
    testOffersWhenFrozen(FeatureBitset features)
    {
        testcase("Offers for Frozen Trust Lines");

        using namespace test::jtx;
        Env env(*this, features);

        Account G1 {"G1"};
        Account A2 {"A2"};
        Account A3 {"A3"};
        Account A4 {"A4"};

        env.fund(XRP(1000), G1, A3, A4);
        env.fund(XRP(2000), A2);
        env.close();

        env.trust(G1["USD"](1000), A2);
        env.trust(G1["USD"](2000), A3);
        env.trust(G1["USD"](2000), A4);
        env.close();

        env(pay(G1, A3, G1["USD"](2000)));
        env(pay(G1, A4, G1["USD"](2000)));
        env.close();

        env(offer(A3, XRP(1000), G1["USD"](1000)), txflags(tfPassive));
        env.close();

        env(pay(A2, G1, G1["USD"](1)), paths(G1["USD"]), sendmax(XRP(1)));
        env.close();

        auto offers = getAccountOffers(env, A3)[jss::offers];
        if(! BEAST_EXPECT(checkArraySize(offers, 1u)))
            return;
        BEAST_EXPECT(
            offers[0u][jss::taker_gets] ==
            G1["USD"](999).value().getJson(JsonOptions::none));

        env(offer(A4, XRP(999), G1["USD"](999)));
        env.close();

        env(trust(G1, A3["USD"](0), tfSetFreeze));
        auto affected =
            env.meta()->getJson(JsonOptions::none)[sfAffectedNodes.fieldName];
        if(! BEAST_EXPECT(checkArraySize(affected, 2u)))
            return;
        auto ff =
            affected[1u][sfModifiedNode.fieldName][sfFinalFields.fieldName];
        BEAST_EXPECT(
            ff[sfHighLimit.fieldName] ==
            G1["USD"](0).value().getJson(JsonOptions::none));
        BEAST_EXPECT(! (ff[jss::Flags].asUInt() & lsfLowFreeze));
        BEAST_EXPECT(ff[jss::Flags].asUInt() & lsfHighFreeze);
        env.close();

        offers = getAccountOffers(env, A3)[jss::offers];
        if(! BEAST_EXPECT(checkArraySize(offers, 1u)))
            return;

        env(pay(A2, G1, G1["USD"](1)), paths(G1["USD"]), sendmax(XRP(1)));
        env.close();

        offers = getAccountOffers(env, A3)[jss::offers];
        if(! BEAST_EXPECT(checkArraySize(offers, 0u)))
            return;

        env(trust(G1, A4["USD"](0), tfSetFreeze));
        affected =
            env.meta()->getJson(JsonOptions::none)[sfAffectedNodes.fieldName];
        if(! BEAST_EXPECT(checkArraySize(affected, 2u)))
            return;
        ff =
            affected[0u][sfModifiedNode.fieldName][sfFinalFields.fieldName];
        BEAST_EXPECT(
            ff[sfLowLimit.fieldName] ==
            G1["USD"](0).value().getJson(JsonOptions::none));
        BEAST_EXPECT(ff[jss::Flags].asUInt() & lsfLowFreeze);
        BEAST_EXPECT(! (ff[jss::Flags].asUInt() & lsfHighFreeze));
        env.close();

        env(offer(A2, G1["USD"](999), XRP(999)));
        affected =
            env.meta()->getJson(JsonOptions::none)[sfAffectedNodes.fieldName];
        if(! BEAST_EXPECT(checkArraySize(affected, 8u)))
            return;
        auto created = affected[5u][sfCreatedNode.fieldName];
        BEAST_EXPECT(created[sfNewFields.fieldName][jss::Account] == A2.human());
        env.close();

        offers = getAccountOffers(env, A4)[jss::offers];
        if(! BEAST_EXPECT(checkArraySize(offers, 0u)))
            return;
    }

public:

    void run() override
    {
        auto testAll = [this](FeatureBitset features)
        {
            testRippleState(features);
            testGlobalFreeze(features);
            testNoFreeze(features);
            testOffersWhenFrozen(features);
        };
        using namespace test::jtx;
        auto const sa = supported_amendments();
        testAll(sa - featureFlow - fix1373 - featureFlowCross);
        testAll(sa               - fix1373 - featureFlowCross);
        testAll(sa                         - featureFlowCross);
        testAll(sa);
    }
};

BEAST_DEFINE_TESTSUITE(Freeze, app, ripple);
} 
























