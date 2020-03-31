

#include <ripple/ledger/ApplyViewImpl.h>
#include <ripple/ledger/PaymentSandbox.h>
#include <test/jtx/PathSet.h>
#include <ripple/ledger/View.h>
#include <ripple/protocol/AmountConversions.h>
#include <ripple/protocol/Feature.h>

namespace ripple {
namespace test {

class PaymentSandbox_test : public beast::unit_test::suite
{
    
    void testSelfFunding (FeatureBitset features)
    {
        testcase ("selfFunding");

        using namespace jtx;
        Env env (*this, features);
        Account const gw1 ("gw1");
        Account const gw2 ("gw2");
        Account const snd ("snd");
        Account const rcv ("rcv");

        env.fund (XRP (10000), snd, rcv, gw1, gw2);

        auto const USD_gw1 = gw1["USD"];
        auto const USD_gw2 = gw2["USD"];

        env.trust (USD_gw1 (10), snd);
        env.trust (USD_gw2 (10), snd);
        env.trust (USD_gw1 (100), rcv);
        env.trust (USD_gw2 (100), rcv);

        env (pay (gw1, snd, USD_gw1 (2)));
        env (pay (gw2, snd, USD_gw2 (4)));

        env (offer (snd, USD_gw1 (2), USD_gw2 (2)),
             txflags (tfPassive));
        env (offer (snd, USD_gw2 (2), USD_gw1 (2)),
             txflags (tfPassive));

        PathSet paths (
            Path (gw1, USD_gw2, gw2),
            Path (gw2, USD_gw1, gw1));

        env (pay (snd, rcv, any (USD_gw1 (4))),
            json (paths.json ()),
            txflags (tfNoRippleDirect | tfPartialPayment));

        env.require (balance ("rcv", USD_gw1 (0)));
        env.require (balance ("rcv", USD_gw2 (2)));
    }

    void testSubtractCredits (FeatureBitset features)
    {
        testcase ("subtractCredits");

        using namespace jtx;
        Env env (*this, features);
        Account const gw1 ("gw1");
        Account const gw2 ("gw2");
        Account const alice ("alice");

        env.fund (XRP (10000), alice, gw1, gw2);

        auto j = env.app().journal ("View");

        auto const USD_gw1 = gw1["USD"];
        auto const USD_gw2 = gw2["USD"];

        env.trust (USD_gw1 (100), alice);
        env.trust (USD_gw2 (100), alice);

        env (pay (gw1, alice, USD_gw1 (50)));
        env (pay (gw2, alice, USD_gw2 (50)));

        STAmount const toCredit (USD_gw1 (30));
        STAmount const toDebit (USD_gw1 (20));
        {
            ApplyViewImpl av (&*env.current(), tapNONE);

            auto const iss = USD_gw1.issue ();
            auto const startingAmount = accountHolds (
                av, alice, iss.currency, iss.account, fhIGNORE_FREEZE, j);

            accountSend (av, gw1, alice, toCredit, j);
            BEAST_EXPECT(accountHolds (av, alice, iss.currency, iss.account,
                        fhIGNORE_FREEZE, j) ==
                    startingAmount + toCredit);

            accountSend (av, alice, gw1, toDebit, j);
            BEAST_EXPECT(accountHolds (av, alice, iss.currency, iss.account,
                        fhIGNORE_FREEZE, j) ==
                    startingAmount + toCredit - toDebit);
        }

        {
            ApplyViewImpl av (&*env.current(), tapNONE);

            auto const iss = USD_gw1.issue ();
            auto const startingAmount = accountHolds (
                av, alice, iss.currency, iss.account, fhIGNORE_FREEZE, j);

            rippleCredit (av, gw1, alice, toCredit, true, j);
            BEAST_EXPECT(accountHolds (av, alice, iss.currency, iss.account,
                        fhIGNORE_FREEZE, j) ==
                    startingAmount + toCredit);

            rippleCredit (av, alice, gw1, toDebit, true, j);
            BEAST_EXPECT(accountHolds (av, alice, iss.currency, iss.account,
                        fhIGNORE_FREEZE, j) ==
                    startingAmount + toCredit - toDebit);
        }

        {
            ApplyViewImpl av (&*env.current(), tapNONE);
            PaymentSandbox pv (&av);

            auto const iss = USD_gw1.issue ();
            auto const startingAmount = accountHolds (
                pv, alice, iss.currency, iss.account, fhIGNORE_FREEZE, j);

            accountSend (pv, gw1, alice, toCredit, j);
            BEAST_EXPECT(accountHolds (pv, alice, iss.currency, iss.account,
                        fhIGNORE_FREEZE, j) ==
                    startingAmount);

            accountSend (pv, alice, gw1, toDebit, j);
            BEAST_EXPECT(accountHolds (pv, alice, iss.currency, iss.account,
                        fhIGNORE_FREEZE, j) ==
                    startingAmount - toDebit);
        }

        {
            ApplyViewImpl av (&*env.current(), tapNONE);
            PaymentSandbox pv (&av);

            auto const iss = USD_gw1.issue ();
            auto const startingAmount = accountHolds (
                pv, alice, iss.currency, iss.account, fhIGNORE_FREEZE, j);

            rippleCredit (pv, gw1, alice, toCredit, true, j);
            BEAST_EXPECT(accountHolds (pv, alice, iss.currency, iss.account,
                        fhIGNORE_FREEZE, j) ==
                    startingAmount);
        }

        {
            ApplyViewImpl av (&*env.current(), tapNONE);
            PaymentSandbox pv (&av);

            auto const iss = USD_gw1.issue ();
            auto const startingAmount = accountHolds (
                pv, alice, iss.currency, iss.account, fhIGNORE_FREEZE, j);

            redeemIOU (pv, alice, toDebit, iss, j);
            BEAST_EXPECT(accountHolds (pv, alice, iss.currency, iss.account,
                        fhIGNORE_FREEZE, j) ==
                    startingAmount - toDebit);
        }

        {
            ApplyViewImpl av (&*env.current(), tapNONE);
            PaymentSandbox pv (&av);

            auto const iss = USD_gw1.issue ();
            auto const startingAmount = accountHolds (
                pv, alice, iss.currency, iss.account, fhIGNORE_FREEZE, j);

            issueIOU (pv, alice, toCredit, iss, j);
            BEAST_EXPECT(accountHolds (pv, alice, iss.currency, iss.account,
                        fhIGNORE_FREEZE, j) ==
                    startingAmount);
        }

        {
            ApplyViewImpl av (&*env.current(), tapNONE);
            PaymentSandbox pv (&av);

            auto const iss = USD_gw1.issue ();
            auto const startingAmount = accountHolds (
                pv, alice, iss.currency, iss.account, fhIGNORE_FREEZE, j);

            accountSend (pv, gw1, alice, toCredit, j);
            BEAST_EXPECT(accountHolds (pv, alice, iss.currency, iss.account,
                        fhIGNORE_FREEZE, j) ==
                    startingAmount);

            {
                PaymentSandbox pv2(&pv);
                BEAST_EXPECT(accountHolds (pv2, alice, iss.currency, iss.account,
                            fhIGNORE_FREEZE, j) ==
                        startingAmount);
                accountSend (pv2, gw1, alice, toCredit, j);
                BEAST_EXPECT(accountHolds (pv2, alice, iss.currency, iss.account,
                            fhIGNORE_FREEZE, j) ==
                        startingAmount);
            }

            accountSend (pv, alice, gw1, toDebit, j);
            BEAST_EXPECT(accountHolds (pv, alice, iss.currency, iss.account,
                        fhIGNORE_FREEZE, j) ==
                    startingAmount - toDebit);
        }
    }

    void testTinyBalance (FeatureBitset features)
    {
        testcase ("Tiny balance");


        using namespace jtx;

        Env env (*this, features);

        Account const gw ("gw");
        Account const alice ("alice");
        auto const USD = gw["USD"];

        auto const issue = USD.issue ();
        STAmount tinyAmt (issue, STAmount::cMinValue, STAmount::cMinOffset + 1,
            false, false, STAmount::unchecked{});
        STAmount hugeAmt (issue, STAmount::cMaxValue, STAmount::cMaxOffset - 1,
            false, false, STAmount::unchecked{});

        for (auto d : {-1, 1})
        {
            auto const closeTime = fix1141Time () +
                d * env.closed()->info().closeTimeResolution;
            env.close (closeTime);
            ApplyViewImpl av (&*env.current (), tapNONE);
            PaymentSandbox pv (&av);
            pv.creditHook (gw, alice, hugeAmt, -tinyAmt);
            if (closeTime > fix1141Time ())
                BEAST_EXPECT(pv.balanceHook (alice, gw, hugeAmt) == tinyAmt);
            else
                BEAST_EXPECT(pv.balanceHook (alice, gw, hugeAmt) != tinyAmt);
        }
    }

    void testReserve(FeatureBitset features)
    {
        testcase ("Reserve");
        using namespace jtx;

        auto accountFundsXRP = [](ReadView const& view,
            AccountID const& id, beast::Journal j) -> XRPAmount
        {
            return toAmount<XRPAmount> (accountHolds (
                view, id, xrpCurrency (), xrpAccount (), fhZERO_IF_FROZEN, j));
        };

        auto reserve = [](jtx::Env& env, std::uint32_t count) -> XRPAmount
        {
            return env.current ()->fees ().accountReserve (count);
        };

        Env env (*this, features);

        Account const alice ("alice");
        env.fund (reserve(env, 1), alice);

        auto const closeTime = fix1141Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);
        ApplyViewImpl av (&*env.current (), tapNONE);
        PaymentSandbox sb (&av);
        {

            accountSend (sb, xrpAccount (), alice, XRP(100), env.journal);
            accountSend (sb, alice, xrpAccount (), XRP(100), env.journal);
            BEAST_EXPECT(
                accountFundsXRP (sb, alice, env.journal) == beast::zero);
        }
    }

    void testBalanceHook(FeatureBitset features)
    {
        testcase ("balanceHook");

        using namespace jtx;
        Env env (*this, features);

        Account const gw ("gw");
        auto const USD = gw["USD"];
        Account const alice ("alice");

        auto const closeTime = fix1274Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        ApplyViewImpl av (&*env.current (), tapNONE);
        PaymentSandbox sb (&av);

        Issue tlIssue = noIssue();
        tlIssue.currency = USD.issue().currency;

        sb.creditHook (gw.id(), alice.id(), {USD, 400}, {tlIssue, 600});
        sb.creditHook (gw.id(), alice.id(), {USD, 100}, {tlIssue, 600});

        STAmount const balance =
            sb.balanceHook (gw.id(), alice.id(), {USD, 600});
        BEAST_EXPECT (balance.getIssuer() == USD.issue().account);
    }

public:
    void run () override
    {
        auto testAll = [this](FeatureBitset features) {
            testSelfFunding(features);
            testSubtractCredits(features);
            testTinyBalance(features);
            testReserve(features);
            testBalanceHook(features);
        };
        using namespace jtx;
        auto const sa = supported_amendments();
        testAll(sa - featureFlow - fix1373 - featureFlowCross);
        testAll(sa                         - featureFlowCross);
        testAll(sa);
    }
};

BEAST_DEFINE_TESTSUITE (PaymentSandbox, ledger, ripple);

}  
}  
























