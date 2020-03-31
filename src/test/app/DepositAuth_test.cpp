

#include <ripple/protocol/Feature.h>
#include <test/jtx.h>

namespace ripple {
namespace test {

static XRPAmount reserve (jtx::Env& env, std::uint32_t count)
{
    return env.current()->fees().accountReserve (count);
}

static bool hasDepositAuth (jtx::Env const& env, jtx::Account const& acct)
{
    return ((*env.le(acct))[sfFlags] & lsfDepositAuth) == lsfDepositAuth;
}


struct DepositAuth_test : public beast::unit_test::suite
{
    void testEnable()
    {
        testcase ("Enable");

        using namespace jtx;
        Account const alice {"alice"};

        {
            Env env (*this, supported_amendments() - featureDepositAuth);
            env.fund (XRP (10000), alice);

            env (fset (alice, asfDepositAuth));
            env.close();
            BEAST_EXPECT (! hasDepositAuth (env, alice));

            env (fclear (alice, asfDepositAuth));
            env.close();
            BEAST_EXPECT (! hasDepositAuth (env, alice));
        }
        {
            Env env (*this);
            env.fund (XRP (10000), alice);

            env (fset (alice, asfDepositAuth));
            env.close();
            BEAST_EXPECT (hasDepositAuth (env, alice));

            env (fclear (alice, asfDepositAuth));
            env.close();
            BEAST_EXPECT (! hasDepositAuth (env, alice));
        }
    }

    void testPayIOU()
    {
        testcase ("Pay IOU");

        using namespace jtx;
        Account const alice {"alice"};
        Account const bob {"bob"};
        Account const carol {"carol"};
        Account const gw {"gw"};
        IOU const USD = gw["USD"];

        Env env (*this);

        env.fund (XRP (10000), alice, bob, carol, gw);
        env.trust (USD (1000), alice, bob);
        env.close();

        env (pay (gw, alice, USD (150)));
        env (offer (carol, USD(100), XRP(100)));
        env.close();

        env (pay (alice, bob, USD (50)));
        env.close();

        env (fset (bob, asfDepositAuth), require(flags (bob, asfDepositAuth)));
        env.close();

        auto failedIouPayments = [this, &env, &alice, &bob, &USD] ()
        {
            env.require (flags (bob, asfDepositAuth));

            PrettyAmount const bobXrpBalance {env.balance (bob, XRP)};
            PrettyAmount const bobUsdBalance {env.balance (bob, USD)};

            env (pay (alice, bob, USD (50)), ter (tecNO_PERMISSION));
            env.close();

            env (pay (alice, bob, drops(1)),
                sendmax (USD (1)), ter (tecNO_PERMISSION));
            env.close();

            BEAST_EXPECT (bobXrpBalance == env.balance (bob, XRP));
            BEAST_EXPECT (bobUsdBalance == env.balance (bob, USD));
        };

        failedIouPayments();

        env (pay (bob, alice, USD(25)));
        env.close();

        {
            STAmount const bobPaysXRP {
                env.balance (bob, XRP) - reserve (env, 1)};
            XRPAmount const bobPaysFee {reserve (env, 1) - reserve (env, 0)};
            env (pay (bob, alice, bobPaysXRP), fee (bobPaysFee));
            env.close();
        }

        BEAST_EXPECT (env.balance (bob, XRP) == reserve (env, 0));
        BEAST_EXPECT (env.balance (bob, USD) == USD(25));
        failedIouPayments();

        env (noop (bob), fee (reserve (env, 0)));
        env.close ();

        BEAST_EXPECT (env.balance (bob, XRP) == XRP (0));
        failedIouPayments();

        env (pay (alice, bob, drops(env.current()->fees().base)));

        env (fclear (bob, asfDepositAuth));
        env.close();

        env (pay (alice, bob, USD (50)));
        env.close();

        env (pay (alice, bob, drops(1)), sendmax (USD (1)));
        env.close();
    }

    void testPayXRP()
    {
        testcase ("Pay XRP");

        using namespace jtx;
        Account const alice {"alice"};
        Account const bob {"bob"};

        Env env (*this);

        env.fund (XRP (10000), alice, bob);

        env (fset (bob, asfDepositAuth), fee (drops (10)));
        env.close();
        BEAST_EXPECT (env.balance (bob, XRP) == XRP (10000) - drops(10));

        env (pay (alice, bob, drops(1)), ter (tecNO_PERMISSION));
        env.close();
        BEAST_EXPECT (env.balance (bob, XRP) == XRP (10000) - drops(10));

        {
            STAmount const bobPaysXRP {
                env.balance (bob, XRP) - reserve (env, 1)};
            XRPAmount const bobPaysFee {reserve (env, 1) - reserve (env, 0)};
            env (pay (bob, alice, bobPaysXRP), fee (bobPaysFee));
            env.close();
        }

        BEAST_EXPECT (env.balance (bob, XRP) == reserve (env, 0));
        env (pay (alice, bob, drops(1)));
        env.close();

        BEAST_EXPECT (env.balance (bob, XRP) == reserve (env, 0) + drops(1));
        env (pay (alice, bob, drops(1)), ter (tecNO_PERMISSION));
        env.close();

        env (noop (bob), fee (reserve (env, 0) + drops(1)));
        env.close ();
        BEAST_EXPECT (env.balance (bob, XRP) == drops(0));

        env (pay (alice, bob, reserve (env, 0) + drops(1)),
            ter (tecNO_PERMISSION));
        env.close();

        env (pay (alice, bob, reserve (env, 0) + drops(0)));
        env.close();
        BEAST_EXPECT (env.balance (bob, XRP) == reserve (env, 0));

        env (pay (alice, bob, reserve (env, 0) + drops(0)));
        env.close();
        BEAST_EXPECT (env.balance (bob, XRP) ==
            (reserve (env, 0) + reserve (env, 0)));

        env (pay (alice, bob, drops(1)), ter (tecNO_PERMISSION));
        env.close();
        BEAST_EXPECT (env.balance (bob, XRP) ==
            (reserve (env, 0) + reserve (env, 0)));

        env (noop (bob), fee (env.balance (bob, XRP)));
        env.close();
        BEAST_EXPECT (env.balance (bob, XRP) == drops(0));

        env (fclear (bob, asfDepositAuth), ter (terINSUF_FEE_B));
        env.close();

        env (pay (alice, bob, drops(1)));
        env.close();
        BEAST_EXPECT (env.balance (bob, XRP) == drops(1));

        env (pay (alice, bob, drops(9)));
        env.close();

        BEAST_EXPECT (env.balance (bob, XRP) == drops(0));
        env.require (nflags (bob, asfDepositAuth));

        env (pay (alice, bob, reserve (env, 0) + drops(1)));
        env.close();
        BEAST_EXPECT (env.balance (bob, XRP) == reserve (env, 0) + drops(1));
    }

    void testNoRipple()
    {
        testcase ("No Ripple");

        using namespace jtx;
        Account const gw1 ("gw1");
        Account const gw2 ("gw2");
        Account const alice ("alice");
        Account const bob ("bob");

        IOU const USD1 (gw1["USD"]);
        IOU const USD2 (gw2["USD"]);

        auto testIssuer = [&] (FeatureBitset const& features,
            bool noRipplePrev,
            bool noRippleNext,
            bool withDepositAuth)
        {
            assert(!withDepositAuth || features[featureDepositAuth]);

            Env env(*this, features);

            env.fund(XRP(10000), gw1, alice, bob);
            env (trust (gw1, alice["USD"](10), noRipplePrev ? tfSetNoRipple : 0));
            env (trust (gw1, bob["USD"](10), noRippleNext ? tfSetNoRipple : 0));
            env.trust(USD1 (10), alice, bob);

            env(pay(gw1, alice, USD1(10)));

            if (withDepositAuth)
                env(fset(gw1, asfDepositAuth));

            TER const result = (noRippleNext && noRipplePrev)
                ? TER {tecPATH_DRY}
                : TER {tesSUCCESS};
            env (pay (alice, bob, USD1(10)), path (gw1), ter (result));
        };

        auto testNonIssuer = [&] (FeatureBitset const& features,
            bool noRipplePrev,
            bool noRippleNext,
            bool withDepositAuth)
        {
            assert(!withDepositAuth || features[featureDepositAuth]);

            Env env(*this, features);

            env.fund(XRP(10000), gw1, gw2, alice);
            env (trust (alice, USD1(10), noRipplePrev ? tfSetNoRipple : 0));
            env (trust (alice, USD2(10), noRippleNext ? tfSetNoRipple : 0));
            env(pay(gw2, alice, USD2(10)));

            if (withDepositAuth)
                env(fset(alice, asfDepositAuth));

            TER const result = (noRippleNext && noRipplePrev)
                ? TER {tecPATH_DRY}
                : TER {tesSUCCESS};
            env (pay (gw1, gw2, USD2 (10)),
                path (alice), sendmax (USD1 (10)), ter (result));
        };

        for (int i = 0; i < 8; ++i)
        {
            auto const noRipplePrev = i & 0x1;
            auto const noRippleNext = i & 0x2;
            auto const withDepositAuth = i & 0x4;
            testIssuer(
                supported_amendments() | featureDepositAuth,
                noRipplePrev,
                noRippleNext,
                withDepositAuth);

            if (!withDepositAuth)
                testIssuer(
                    supported_amendments() - featureDepositAuth,
                    noRipplePrev,
                    noRippleNext,
                    withDepositAuth);

            testNonIssuer(
                supported_amendments() | featureDepositAuth,
                noRipplePrev,
                noRippleNext,
                withDepositAuth);

            if (!withDepositAuth)
                testNonIssuer(
                    supported_amendments() - featureDepositAuth,
                    noRipplePrev,
                    noRippleNext,
                    withDepositAuth);
        }
    }

    void run() override
    {
        testEnable();
        testPayIOU();
        testPayXRP();
        testNoRipple();
    }
};

struct DepositPreauth_test : public beast::unit_test::suite
{
    void testEnable()
    {
        testcase ("Enable");

        using namespace jtx;
        Account const alice {"alice"};
        Account const becky {"becky"};
        {
            Env env (*this, supported_amendments() - featureDepositPreauth);
            env.fund (XRP (10000), alice, becky);
            env.close();

            env (deposit::auth (alice, becky), ter (temDISABLED));
            env.close();
            env.require (owners (alice, 0));
            env.require (owners (becky, 0));

            env (deposit::unauth (alice, becky), ter (temDISABLED));
            env.close();
            env.require (owners (alice, 0));
            env.require (owners (becky, 0));
        }
        {
            Env env (*this);
            env.fund (XRP (10000), alice, becky);
            env.close();

            env (deposit::auth (alice, becky));
            env.close();
            env.require (owners (alice, 1));
            env.require (owners (becky, 0));

            env (deposit::unauth (alice, becky));
            env.close();
            env.require (owners (alice, 0));
            env.require (owners (becky, 0));
        }
    }

    void testInvalid()
    {
        testcase ("Invalid");

        using namespace jtx;
        Account const alice {"alice"};
        Account const becky {"becky"};
        Account const carol {"carol"};

        Env env (*this);

        env.memoize (alice);
        env.memoize (becky);
        env.memoize (carol);

        env (deposit::auth (alice, becky), seq (1), ter (terNO_ACCOUNT));

        env.fund (XRP (10000), alice, becky);
        env.close();

        env (deposit::auth (alice, becky), fee (drops(-10)), ter (temBAD_FEE));
        env.close();

        env (deposit::auth (alice, becky),
            txflags (tfSell), ter (temINVALID_FLAG));
        env.close();

        {
            Json::Value tx {deposit::auth (alice, becky)};
            tx.removeMember (sfAuthorize.jsonName);
            env (tx, ter (temMALFORMED));
            env.close();
        }
        {
            Json::Value tx {deposit::auth (alice, becky)};
            tx[sfUnauthorize.jsonName] = becky.human();
            env (tx, ter (temMALFORMED));
            env.close();
        }
        {
            Json::Value tx {deposit::auth (alice, becky)};
            tx[sfAuthorize.jsonName] = to_string (xrpAccount());
            env (tx, ter (temINVALID_ACCOUNT_ID));
            env.close();
        }

        env (deposit::auth (alice, alice), ter (temCANNOT_PREAUTH_SELF));
        env.close();

        env (deposit::auth (alice, carol), ter (tecNO_TARGET));
        env.close();

        env.require (owners (alice, 0));
        env.require (owners (becky, 0));
        env (deposit::auth (alice, becky));
        env.close();
        env.require (owners (alice, 1));
        env.require (owners (becky, 0));

        env (deposit::auth (alice, becky), ter (tecDUPLICATE));
        env.close();
        env.require (owners (alice, 1));
        env.require (owners (becky, 0));

        env.fund (drops (249'999'999), carol);
        env.close();

        env (deposit::auth (carol, becky), ter (tecINSUFFICIENT_RESERVE));
        env.close();
        env.require (owners (carol, 0));
        env.require (owners (becky, 0));

        env (pay (alice, carol, drops (11)));
        env.close();
        env (deposit::auth (carol, becky));
        env.close();
        env.require (owners (carol, 1));
        env.require (owners (becky, 0));

        env (deposit::auth (carol, alice), ter (tecINSUFFICIENT_RESERVE));
        env.close();
        env.require (owners (carol, 1));
        env.require (owners (becky, 0));
        env.require (owners (alice, 1));

        env (deposit::unauth (alice, carol), ter (tecNO_ENTRY));
        env.close();
        env.require (owners (alice, 1));
        env.require (owners (becky, 0));

        env (deposit::unauth (alice, becky));
        env.close();
        env.require (owners (alice, 0));
        env.require (owners (becky, 0));

        env (deposit::unauth (alice, becky), ter (tecNO_ENTRY));
        env.close();
        env.require (owners (alice, 0));
        env.require (owners (becky, 0));
    }

    void testPayment (FeatureBitset features)
    {
        testcase ("Payment");

        using namespace jtx;
        Account const alice {"alice"};
        Account const becky {"becky"};
        Account const gw {"gw"};
        IOU const USD (gw["USD"]);

        bool const supportsPreauth = {features[featureDepositPreauth]};

        {
            Env env (*this, features);
            env.fund (XRP (5000), alice, becky, gw);
            env.close();

            env.trust (USD (1000), alice);
            env.trust (USD (1000), becky);
            env.close();

            env (pay (gw, alice, USD (500)));
            env.close();

            env (offer (alice, XRP (100), USD (100), tfPassive),
                require (offers (alice, 1)));
            env.close();

            env (pay (becky, becky, USD (10)),
                path (~USD), sendmax (XRP (10)));
            env.close();

            env(fset (becky, asfDepositAuth));
            env.close();

            TER const expect {
                supportsPreauth ? TER {tesSUCCESS} : TER {tecNO_PERMISSION}};

            env (pay (becky, becky, USD (10)),
                path (~USD), sendmax (XRP (10)), ter (expect));
            env.close();
        }

        if (supportsPreauth)
        {

            Account const carol {"carol"};

            Env env (*this, features);
            env.fund (XRP (5000), alice, becky, carol, gw);
            env.close();

            env.trust (USD (1000), alice);
            env.trust (USD (1000), becky);
            env.trust (USD (1000), carol);
            env.close();

            env (pay (gw, alice, USD (1000)));
            env.close();

            env (pay (alice, becky, XRP (100)));
            env (pay (alice, becky, USD (100)));
            env.close();

            env(fset (becky, asfDepositAuth));
            env.close();

            env (pay (alice, becky, XRP (100)), ter (tecNO_PERMISSION));
            env (pay (alice, becky, USD (100)), ter (tecNO_PERMISSION));
            env.close();

            env (deposit::auth (becky, carol));
            env.close();

            env (pay (alice, becky, XRP (100)), ter (tecNO_PERMISSION));
            env (pay (alice, becky, USD (100)), ter (tecNO_PERMISSION));
            env.close();

            env (deposit::auth (becky, alice));
            env.close();

            env (pay (alice, becky, XRP (100)));
            env (pay (alice, becky, USD (100)));
            env.close();

            env(fset (alice, asfDepositAuth));
            env.close();

            env (pay (becky, alice, XRP (100)), ter (tecNO_PERMISSION));
            env (pay (becky, alice, USD (100)), ter (tecNO_PERMISSION));
            env.close();

            env (deposit::unauth (becky, carol));
            env.close();

            env (pay (alice, becky, XRP (100)));
            env (pay (alice, becky, USD (100)));
            env.close();

            env (deposit::unauth (becky, alice));
            env.close();

            env (pay (alice, becky, XRP (100)), ter (tecNO_PERMISSION));
            env (pay (alice, becky, USD (100)), ter (tecNO_PERMISSION));
            env.close();

            env(fclear (becky, asfDepositAuth));
            env.close();

            env (pay (alice, becky, XRP (100)));
            env (pay (alice, becky, USD (100)));
            env.close();
        }
    }

    void run() override
    {
        testEnable();
        testInvalid();
        testPayment (jtx::supported_amendments() - featureDepositPreauth);
        testPayment (jtx::supported_amendments());
    }
};

BEAST_DEFINE_TESTSUITE(DepositAuth,app,ripple);
BEAST_DEFINE_TESTSUITE(DepositPreauth,app,ripple);

} 
} 
























