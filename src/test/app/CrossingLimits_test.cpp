

#include <test/jtx.h>
#include <ripple/beast/unit_test.h>
#include <ripple/protocol/Feature.h>

namespace ripple {
namespace test {

class CrossingLimits_test : public beast::unit_test::suite
{
private:
    void
    n_offers (
        jtx::Env& env,
        std::size_t n,
        jtx::Account const& account,
        STAmount const& in,
        STAmount const& out)
    {
        using namespace jtx;
        auto const ownerCount = env.le(account)->getFieldU32(sfOwnerCount);
        for (std::size_t i = 0; i < n; i++)
        {
            env(offer(account, in, out));
            env.close();
        }
        env.require (owners (account, ownerCount + n));
    }

public:

    void
    testStepLimit(FeatureBitset features)
    {
        testcase ("Step Limit");

        using namespace jtx;
        Env env(*this, features);
        auto const closeTime =
            fix1449Time() +
                100 * env.closed()->info().closeTimeResolution;
        env.close (closeTime);

        auto const gw = Account("gateway");
        auto const USD = gw["USD"];

        env.fund(XRP(100000000), gw, "alice", "bob", "carol", "dan");
        env.trust(USD(1), "bob");
        env(pay(gw, "bob", USD(1)));
        env.trust(USD(1), "dan");
        env(pay(gw, "dan", USD(1)));
        n_offers (env, 2000, "bob", XRP(1), USD(1));
        n_offers (env, 1, "dan", XRP(1), USD(1));

        env(offer("alice", USD(1000), XRP(1000)));
        env.require (balance("alice", USD(1)));
        env.require (owners("alice", 2));
        env.require (balance("bob", USD(0)));
        env.require (owners("bob", 1001));
        env.require (balance("dan", USD(1)));
        env.require (owners("dan", 2));

        env(offer("carol", USD(1000), XRP(1000)));
        env.require (balance("carol", USD(none)));
        env.require (owners("carol", 1));
        env.require (balance("bob", USD(0)));
        env.require (owners("bob", 1));
        env.require (balance("dan", USD(1)));
        env.require (owners("dan", 2));
    }

    void
    testCrossingLimit(FeatureBitset features)
    {
        testcase ("Crossing Limit");

        using namespace jtx;
        Env env(*this, features);
        auto const closeTime =
            fix1449Time() +
                100 * env.closed()->info().closeTimeResolution;
        env.close (closeTime);

        auto const gw = Account("gateway");
        auto const USD = gw["USD"];

        int const maxConsumed = features[featureFlowCross] ? 1000 : 850;

        env.fund(XRP(100000000), gw, "alice", "bob", "carol");
        int const bobsOfferCount = maxConsumed + 150;
        env.trust(USD(bobsOfferCount), "bob");
        env(pay(gw, "bob", USD(bobsOfferCount)));
        env.close();
        n_offers (env, bobsOfferCount, "bob", XRP(1), USD(1));

        env(offer("alice", USD(bobsOfferCount), XRP(bobsOfferCount)));
        env.close();
        env.require (balance("alice", USD(maxConsumed)));
        env.require (balance("bob", USD(150)));
        env.require (owners ("bob", 150 + 1));

        env(offer("carol", USD(1000), XRP(1000)));
        env.close();
        env.require (balance("carol", USD(150)));
        env.require (balance("bob", USD(0)));
        env.require (owners ("bob", 1));
    }

    void
    testStepAndCrossingLimit(FeatureBitset features)
    {
        testcase ("Step And Crossing Limit");

        using namespace jtx;
        Env env(*this, features);
        auto const closeTime =
            fix1449Time() +
                100 * env.closed()->info().closeTimeResolution;
        env.close (closeTime);

        auto const gw = Account("gateway");
        auto const USD = gw["USD"];

        env.fund(XRP(100000000), gw, "alice", "bob", "carol", "dan", "evita");

        bool const isFlowCross {features[featureFlowCross]};
        int const maxConsumed = isFlowCross ? 1000 : 850;

        int const evitasOfferCount {maxConsumed + 49};
        env.trust(USD(1000), "alice");
        env(pay(gw, "alice", USD(1000)));
        env.trust(USD(1000), "carol");
        env(pay(gw, "carol", USD(1)));
        env.trust(USD(evitasOfferCount + 1), "evita");
        env(pay(gw, "evita", USD(evitasOfferCount + 1)));

        int const carolsOfferCount {isFlowCross ? 700 : 850};
        n_offers (env, 400, "alice", XRP(1), USD(1));
        n_offers (env, carolsOfferCount, "carol", XRP(1), USD(1));
        n_offers (env, evitasOfferCount, "evita", XRP(1), USD(1));

        env(offer("bob", USD(1000), XRP(1000)));
        env.require (balance("bob", USD(401)));
        env.require (balance("alice", USD(600)));
        env.require (owners("alice", 1));
        env.require (balance("carol", USD(0)));
        env.require (owners("carol", carolsOfferCount - 599));
        env.require (balance("evita", USD(evitasOfferCount + 1)));
        env.require (owners("evita", evitasOfferCount + 1));

        env(offer("dan", USD(maxConsumed + 50), XRP(maxConsumed + 50)));
        env.require (balance("dan", USD(maxConsumed - 100)));
        env.require (owners("dan", 2));
        env.require (balance("alice", USD(600)));
        env.require (owners("alice", 1));
        env.require (balance("carol", USD(0)));
        env.require (owners("carol", 1));
        env.require (balance("evita", USD(150)));
        env.require (owners("evita", 150));
    }

    void testAutoBridgedLimitsTaker (FeatureBitset features)
    {
        testcase ("Auto Bridged Limits Taker");

        using namespace jtx;
        Env env(*this, features);
        auto const closeTime =
            fix1449Time() +
                100 * env.closed()->info().closeTimeResolution;
        env.close (closeTime);

        auto const gw = Account("gateway");
        auto const USD = gw["USD"];
        auto const EUR = gw["EUR"];

        env.fund(XRP(100000000), gw, "alice", "bob", "carol", "dan", "evita");

        env.trust(USD(2000), "alice");
        env(pay(gw, "alice", USD(2000)));
        env.trust(USD(1000), "carol");
        env(pay(gw, "carol", USD(3)));
        env.trust(USD(1000), "evita");
        env(pay(gw, "evita", USD(1000)));

        n_offers (env,  302, "alice", EUR(2), XRP(1));
        n_offers (env,  300, "alice", XRP(1), USD(4));
        n_offers (env,  497, "carol", XRP(1), USD(3));
        n_offers (env, 1001, "evita", EUR(1), USD(1));

        env.trust(EUR(10000), "bob");
        env.close();
        env(pay(gw, "bob", EUR(1000)));
        env.close();
        env(offer("bob", USD(2000), EUR(2000)));
        env.require (balance("bob", USD(1204)));
        env.require (balance("bob", EUR( 397)));

        env.require (balance("alice", USD(800)));
        env.require (balance("alice", EUR(602)));
        env.require (offers("alice", 1));
        env.require (owners("alice", 3));

        env.require (balance("carol", USD(0)));
        env.require (balance("carol", EUR(none)));
        env.require (offers("carol", 100));
        env.require (owners("carol", 101));

        env.require (balance("evita", USD(999)));
        env.require (balance("evita", EUR(1)));
        env.require (offers("evita", 1000));
        env.require (owners("evita", 1002));

        env.trust(EUR(10000), "dan");
        env.close();
        env(pay(gw, "dan", EUR(1000)));
        env.close();

        env(offer("dan", USD(900), EUR(900)));
        env.require (balance("dan", USD(850)));
        env.require (balance("dan", EUR(150)));

        env.require (balance("alice", USD(800)));
        env.require (balance("alice", EUR(602)));
        env.require (offers("alice", 1));
        env.require (owners("alice", 3));

        env.require (balance("carol", USD(0)));
        env.require (balance("carol", EUR(none)));
        env.require (offers("carol", 0));
        env.require (owners("carol", 1));

        env.require (balance("evita", USD(149)));
        env.require (balance("evita", EUR(851)));
        env.require (offers("evita", 150));
        env.require (owners("evita", 152));
    }

    void
    testAutoBridgedLimitsFlowCross(FeatureBitset features)
    {
        testcase("Auto Bridged Limits FlowCross");


        using namespace jtx;

        auto const gw = Account("gateway");
        auto const alice = Account("alice");
        auto const bob = Account("bob");
        auto const carol = Account("carol");

        auto const USD = gw["USD"];
        auto const EUR = gw["EUR"];

        {
            Env env(*this, features);
            auto const closeTime =
                fix1449Time() + 100 * env.closed()->info().closeTimeResolution;
            env.close(closeTime);

            env.fund(XRP(100000000), gw, alice, bob, carol);

            env.trust(USD(4000), alice);
            env(pay(gw, alice, USD(4000)));
            env.trust(USD(1000), carol);
            env(pay(gw, carol, USD(3)));

            n_offers(env, 2000, alice, EUR(2), XRP(1));
            n_offers(env, 300, alice, XRP(1), USD(4));
            n_offers(
                env, 801, carol, XRP(1), USD(3));  
            n_offers(env, 1000, alice, XRP(1), USD(3));

            n_offers(env, 1, alice, EUR(500), USD(500));

            env.trust(EUR(10000), bob);
            env.close();
            env(pay(gw, bob, EUR(2000)));
            env.close();
            env(offer(bob, USD(4000), EUR(4000)));
            env.close();

            env.require(balance(bob, USD(2300)));
            env.require(balance(bob, EUR(500)));
            env.require(offers(bob, 1));
            env.require(owners(bob, 3));

            env.require(balance(alice, USD(1703)));
            env.require(balance(alice, EUR(1500)));
            auto const numAOffers =
                2000 + 300 + 1000 + 1 - (2 * 300 + 2 * 199 + 1 + 1);
            env.require(offers(alice, numAOffers));
            env.require(owners(alice, numAOffers + 2));

            env.require(offers(carol, 0));
        }
        {
            Env env(*this, features);
            auto const closeTime =
                fix1449Time() + 100 * env.closed()->info().closeTimeResolution;
            env.close(closeTime);

            env.fund(XRP(100000000), gw, alice, bob, carol);

            env.trust(USD(4000), alice);
            env(pay(gw, alice, USD(4000)));
            env.trust(USD(1000), carol);
            env(pay(gw, carol, USD(3)));

            n_offers(env, 1, alice, EUR(1), USD(10));
            n_offers(env, 2000, alice, EUR(2), XRP(1));
            n_offers(env, 300, alice, XRP(1), USD(4));
            n_offers(
                env, 801, carol, XRP(1), USD(3));  
            n_offers(env, 1000, alice, XRP(1), USD(3));

            n_offers(env, 1, alice, EUR(499), USD(499));

            env.trust(EUR(10000), bob);
            env.close();
            env(pay(gw, bob, EUR(2000)));
            env.close();
            env(offer(bob, USD(4000), EUR(4000)));
            env.close();

            env.require(balance(bob, USD(2309)));
            env.require(balance(bob, EUR(500)));
            env.require(offers(bob, 1));
            env.require(owners(bob, 3));

            env.require(balance(alice, USD(1694)));
            env.require(balance(alice, EUR(1500)));
            auto const numAOffers =
                1 + 2000 + 300 + 1000 + 1 - (1 + 2 * 300 + 2 * 199 + 1 + 1);
            env.require(offers(alice, numAOffers));
            env.require(owners(alice, numAOffers + 2));

            env.require(offers(carol, 0));
        }
    }

    void testAutoBridgedLimits (FeatureBitset features)
    {
        if (features[featureFlowCross])
            testAutoBridgedLimitsFlowCross (features);
        else
            testAutoBridgedLimitsTaker (features);
    }

    void
    testOfferOverflow (FeatureBitset features)
    {
        testcase("Offer Overflow");

        using namespace jtx;

        auto const gw = Account("gateway");
        auto const alice = Account("alice");
        auto const bob = Account("bob");

        auto const USD = gw["USD"];

        Env env(*this, features);
        auto const closeTime =
            fix1449Time() + 100 * env.closed()->info().closeTimeResolution;
        env.close(closeTime);

        env.fund(XRP(100000000), gw, alice, bob);

        env.trust(USD(8000), alice);
        env.trust(USD(8000), bob);
        env.close();

        env(pay(gw, alice, USD(8000)));
        env.close();

        n_offers(env, 998, alice, XRP(1.00), USD(1));
        n_offers(env, 998, alice, XRP(0.99), USD(1));
        n_offers(env, 998, alice, XRP(0.98), USD(1));
        n_offers(env, 998, alice, XRP(0.97), USD(1));
        n_offers(env, 998, alice, XRP(0.96), USD(1));
        n_offers(env, 998, alice, XRP(0.95), USD(1));

        bool const withFlowCross = features[featureFlowCross];
        env(offer(bob, USD(8000), XRP(8000)), ter(withFlowCross ? TER{tecOVERSIZE} : tesSUCCESS));
        env.close();

        env.require(balance(bob, USD(withFlowCross ? 0 : 850)));
    }

    void
    run() override
    {
        auto testAll = [this](FeatureBitset features) {
            testStepLimit(features);
            testCrossingLimit(features);
            testStepAndCrossingLimit(features);
            testAutoBridgedLimits(features);
            testOfferOverflow(features);
        };
        using namespace jtx;
        auto const sa = supported_amendments();
        testAll(sa - featureFlow - fix1373 - featureFlowCross);
        testAll(sa               - fix1373 - featureFlowCross);
        testAll(sa                         - featureFlowCross);
        testAll(sa                                           );
    }
};

BEAST_DEFINE_TESTSUITE_MANUAL_PRIO(CrossingLimits,tx,ripple,10);

} 
} 
























