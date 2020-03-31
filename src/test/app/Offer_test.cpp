

#include <test/jtx.h>
#include <test/jtx/WSClient.h>
#include <test/jtx/PathSet.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/jss.h>
#include <ripple/protocol/Quality.h>

namespace ripple {
namespace test {

class Offer_test : public beast::unit_test::suite
{
    XRPAmount reserve(jtx::Env& env, std::uint32_t count)
    {
        return env.current()->fees().accountReserve (count);
    }

    std::uint32_t lastClose (jtx::Env& env)
    {
        return env.current()->info().parentCloseTime.time_since_epoch().count();
    }

    static auto
    xrpMinusFee (jtx::Env const& env, std::int64_t xrpAmount)
        -> jtx::PrettyAmount
    {
        using namespace jtx;
        auto feeDrops = env.current()->fees().base;
        return drops (dropsPerXRP<std::int64_t>::value * xrpAmount - feeDrops);
    }

    static auto
    ledgerEntryState(jtx::Env & env, jtx::Account const& acct_a,
        jtx::Account const & acct_b, std::string const& currency)
    {
        Json::Value jvParams;
        jvParams[jss::ledger_index] = "current";
        jvParams[jss::ripple_state][jss::currency] = currency;
        jvParams[jss::ripple_state][jss::accounts] = Json::arrayValue;
        jvParams[jss::ripple_state][jss::accounts].append(acct_a.human());
        jvParams[jss::ripple_state][jss::accounts].append(acct_b.human());
        return env.rpc ("json", "ledger_entry",
            to_string(jvParams))[jss::result];
    }

    static auto
    ledgerEntryRoot (jtx::Env & env, jtx::Account const& acct)
    {
        Json::Value jvParams;
        jvParams[jss::ledger_index] = "current";
        jvParams[jss::account_root] = acct.human();
        return env.rpc ("json", "ledger_entry",
            to_string(jvParams))[jss::result];
    }

    static auto
    ledgerEntryOffer (jtx::Env & env,
        jtx::Account const& acct, std::uint32_t offer_seq)
    {
        Json::Value jvParams;
        jvParams[jss::offer][jss::account] = acct.human();
        jvParams[jss::offer][jss::seq] = offer_seq;
        return env.rpc ("json", "ledger_entry",
            to_string(jvParams))[jss::result];
    }

    static auto
    getBookOffers(jtx::Env & env,
        Issue const& taker_pays, Issue const& taker_gets)
    {
        Json::Value jvbp;
        jvbp[jss::ledger_index] = "current";
        jvbp[jss::taker_pays][jss::currency] = to_string(taker_pays.currency);
        jvbp[jss::taker_pays][jss::issuer] = to_string(taker_pays.account);
        jvbp[jss::taker_gets][jss::currency] = to_string(taker_gets.currency);
        jvbp[jss::taker_gets][jss::issuer] = to_string(taker_gets.account);
        return env.rpc("json", "book_offers", to_string(jvbp)) [jss::result];
    }

public:
    void testRmFundedOffer (FeatureBitset features)
    {
        testcase ("Incorrect Removal of Funded Offers");


        using namespace jtx;
        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        auto const gw = Account {"gateway"};
        auto const USD = gw["USD"];
        auto const BTC = gw["BTC"];
        Account const alice {"alice"};
        Account const bob {"bob"};
        Account const carol {"carol"};

        env.fund (XRP (10000), alice, bob, carol, gw);
        env.trust (USD (1000), alice, bob, carol);
        env.trust (BTC (1000), alice, bob, carol);

        env (pay (gw, alice, BTC (1000)));

        env (pay (gw, carol, USD (1000)));
        env (pay (gw, carol, BTC (1000)));

        env (offer (carol, BTC (49), XRP (49)));
        env (offer (carol, BTC (51), XRP (51)));

        env (offer (carol, XRP (50), USD (50)));
        env (offer (carol, XRP (50), USD (50)));

        env (offer (carol, BTC (1), USD (100)));

        PathSet paths (Path (XRP, USD), Path (USD));

        env (pay (alice, bob, USD (100)), json (paths.json ()),
            sendmax (BTC (1000)), txflags (tfPartialPayment));

        env.require (balance (bob, USD (100)));
        BEAST_EXPECT(!isOffer (env, carol, BTC (1), USD (100)) &&
            isOffer (env, carol, BTC (49), XRP (49)));
    }

    void testCanceledOffer (FeatureBitset features)
    {
        testcase ("Removing Canceled Offers");

        using namespace jtx;
        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        auto const gw = Account {"gateway"};
        auto const alice = Account {"alice"};
        auto const USD = gw["USD"];

        env.fund (XRP (10000), alice, gw);
        env.close();
        env.trust (USD (100), alice);
        env.close();

        env (pay (gw, alice, USD (50)));
        env.close();

        auto const offer1Seq = env.seq (alice);

        env (offer (alice, XRP (500), USD (100)),
            require (offers (alice, 1)));
        env.close();

        BEAST_EXPECT(isOffer (env, alice, XRP (500), USD (100)));

        auto const offer2Seq = env.seq (alice);

        env (offer (alice, XRP (300), USD (100)),
             json (jss::OfferSequence, offer1Seq),
             require (offers (alice, 1)));
        env.close();

        BEAST_EXPECT(isOffer (env, alice, XRP (300), USD (100)) &&
            !isOffer (env, alice, XRP (500), USD (100)));


        env (offer (alice, XRP (400), USD (200)),
             json (jss::OfferSequence, offer1Seq),
             require (offers (alice, 2)));
        env.close();

        BEAST_EXPECT(isOffer (env, alice, XRP (300), USD (100)) &&
            isOffer (env, alice, XRP (400), USD (200)));

        auto const offer4Seq = env.seq (alice);
        env (offer (alice, XRP (222), USD (111)),
            require (offers (alice, 3)));
        env.close();

        BEAST_EXPECT(isOffer (env, alice, XRP (222), USD (111)));
        {
            Json::Value cancelOffer;
            cancelOffer[jss::Account] = alice.human();
            cancelOffer[jss::OfferSequence] = offer4Seq;
            cancelOffer[jss::TransactionType] = jss::OfferCancel;
            env (cancelOffer);
        }
        env.close();
        BEAST_EXPECT(env.seq(alice) == offer4Seq + 2);

        BEAST_EXPECT(!isOffer (env, alice, XRP (222), USD (111)));

        env.require (offers (alice, 2));

        bool const featPreauth {features[featureDepositPreauth]};
        env (offer (alice, XRP (5), USD (2)),
            json (sfExpiration.fieldName, lastClose(env)),
            json (jss::OfferSequence, offer2Seq),
            ter (featPreauth ? TER {tecEXPIRED} : TER {tesSUCCESS}));
        env.close();

        env.require (offers (alice, 2));
        BEAST_EXPECT( isOffer (env, alice, XRP (300), USD (100))); 
        BEAST_EXPECT(!isOffer (env, alice, XRP   (5), USD   (2))); 
    }

    void testTinyPayment (FeatureBitset features)
    {
        testcase ("Tiny payments");

        using namespace jtx;
        using namespace std::chrono_literals;
        auto const alice = Account {"alice"};
        auto const bob = Account {"bob"};
        auto const carol = Account {"carol"};
        auto const gw = Account {"gw"};

        auto const USD = gw["USD"];
        auto const EUR = gw["EUR"];

        Env env {*this, features};

        env.fund (XRP (10000), alice, bob, carol, gw);
        env.trust (USD (1000), alice, bob, carol);
        env.trust (EUR (1000), alice, bob, carol);
        env (pay (gw, alice, USD (100)));
        env (pay (gw, carol, EUR (100)));

        for (int i=0;i<101;++i)
            env (offer (carol, USD (1), EUR (2)));

        auto hasFeature = [](Env& env, uint256 const& f)
        {
            return (env.app().config().features.find(f) !=
                env.app().config().features.end());
        };

        for (auto d : {-1, 1})
        {
            auto const closeTime = STAmountSO::soTime +
                d * env.closed()->info().closeTimeResolution;
            env.close (closeTime);
            *stAmountCalcSwitchover = closeTime > STAmountSO::soTime ||
                !hasFeature(env, fix1513);
            TER const expectedResult = *stAmountCalcSwitchover ?
                TER {tesSUCCESS} : TER {tecPATH_PARTIAL};
            env (pay (alice, bob, EUR (epsilon)), path (~EUR),
                sendmax (USD (100)), ter (expectedResult));
        }
    }

    void testXRPTinyPayment (FeatureBitset features)
    {
        testcase ("XRP Tiny payments");


        using namespace jtx;
        using namespace std::chrono_literals;
        auto const alice = Account {"alice"};
        auto const bob = Account {"bob"};
        auto const carol = Account {"carol"};
        auto const dan = Account {"dan"};
        auto const erin = Account {"erin"};
        auto const gw = Account {"gw"};

        auto const USD = gw["USD"];

        for (auto withFix : {false, true})
        {
            if (!withFix)
                continue;

            Env env {*this, features};

            auto closeTime = [&]
            {
                auto const delta =
                    100 * env.closed ()->info ().closeTimeResolution;
                if (withFix)
                    return STAmountSO::soTime2 + delta;
                else
                    return STAmountSO::soTime2 - delta;
            }();

            env.fund (XRP (10000), alice, bob, carol, dan, erin, gw);
            env.trust (USD (1000), alice, bob, carol, dan, erin);
            env (pay (gw, carol, USD (0.99999)));
            env (pay (gw, dan, USD (1)));
            env (pay (gw, erin, USD (1)));

            env (offer (carol, drops (1), USD (1)));
            env (offer (dan, XRP (100), USD (1)));

            env.close (closeTime);
            env (offer (erin, drops (1), USD (1)));

            {
                env (pay (alice, bob, USD (1)), path (~USD),
                    sendmax (XRP (102)),
                    txflags (tfNoRippleDirect | tfPartialPayment));

                env.require (
                    offers (carol, 0),
                    offers (dan, 1));
                if (!withFix)
                {
                    env.require (
                        balance (erin, USD (1)),
                        offers (erin, 0));
                }
                else
                {
                    env.require (
                        balance (erin, USD (0.99999)),
                        offers (erin, 1));
                }
            }
        }
    }

    void testEnforceNoRipple (FeatureBitset features)
    {
        testcase ("Enforce No Ripple");

        using namespace jtx;

        auto const gw = Account {"gateway"};
        auto const USD = gw["USD"];
        auto const BTC = gw["BTC"];
        auto const EUR = gw["EUR"];
        Account const alice {"alice"};
        Account const bob {"bob"};
        Account const carol {"carol"};
        Account const dan {"dan"};

        {
            Env env {*this, features};
            auto const closeTime =
                fix1449Time () +
                    100 * env.closed ()->info ().closeTimeResolution;
            env.close (closeTime);

            auto const gw1 = Account {"gw1"};
            auto const USD1 = gw1["USD"];
            auto const gw2 = Account {"gw2"};
            auto const USD2 = gw2["USD"];

            env.fund (XRP (10000), alice, noripple (bob), carol, dan, gw1, gw2);
            env.trust (USD1 (1000), alice, carol, dan);
            env(trust (bob, USD1 (1000), tfSetNoRipple));
            env.trust (USD2 (1000), alice, carol, dan);
            env(trust (bob, USD2 (1000), tfSetNoRipple));

            env (pay (gw1, dan, USD1 (50)));
            env (pay (gw1, bob, USD1 (50)));
            env (pay (gw2, bob, USD2 (50)));

            env (offer (dan, XRP (50), USD1 (50)));

            env (pay (alice, carol, USD2 (50)), path (~USD1, bob),
                sendmax (XRP (50)), txflags (tfNoRippleDirect),
                ter(tecPATH_DRY));
        }
        {
            Env env {*this, features};
            auto const closeTime =
                fix1449Time () +
                    100 * env.closed ()->info ().closeTimeResolution;
            env.close (closeTime);

            auto const gw1 = Account {"gw1"};
            auto const USD1 = gw1["USD"];
            auto const gw2 = Account {"gw2"};
            auto const USD2 = gw2["USD"];

            env.fund (XRP (10000), alice, bob, carol, dan, gw1, gw2);
            env.trust (USD1 (1000), alice, bob, carol, dan);
            env.trust (USD2 (1000), alice, bob, carol, dan);

            env (pay (gw1, dan, USD1 (50)));
            env (pay (gw1, bob, USD1 (50)));
            env (pay (gw2, bob, USD2 (50)));

            env (offer (dan, XRP (50), USD1 (50)));

            env (pay (alice, carol, USD2 (50)), path (~USD1, bob),
                sendmax (XRP (50)), txflags (tfNoRippleDirect));

            env.require (balance (alice, xrpMinusFee (env, 10000 - 50)));
            env.require (balance (bob, USD1 (100)));
            env.require (balance (bob, USD2 (0)));
            env.require (balance (carol, USD2 (50)));
        }
    }

    void
    testInsufficientReserve (FeatureBitset features)
    {
        testcase ("Insufficient Reserve");


        using namespace jtx;

        auto const gw = Account {"gateway"};
        auto const alice = Account {"alice"};
        auto const bob = Account {"bob"};
        auto const carol = Account {"carol"};
        auto const USD = gw["USD"];

        auto const usdOffer = USD (1000);
        auto const xrpOffer = XRP (1000);

        {
            Env env {*this, features};
            auto const closeTime =
                fix1449Time () +
                    100 * env.closed ()->info ().closeTimeResolution;
            env.close (closeTime);

            env.fund (XRP (1000000), gw);

            auto const f = env.current ()->fees ().base;
            auto const r = reserve (env, 0);

            env.fund (r + f, alice);

            env (trust (alice, usdOffer),           ter(tesSUCCESS));
            env (pay (gw, alice, usdOffer),         ter(tesSUCCESS));
            env (offer (alice, xrpOffer, usdOffer),
                ter(tecINSUF_RESERVE_OFFER));

            env.require (
                balance (alice, r - f),
                owners (alice, 1));
        }

        {
            Env env {*this, features};
            auto const closeTime =
                fix1449Time () +
                    100 * env.closed ()->info ().closeTimeResolution;
            env.close (closeTime);

            env.fund (XRP (1000000), gw);

            auto const f = env.current ()->fees ().base;
            auto const r = reserve (env, 0);

            auto const usdOffer2 = USD (500);
            auto const xrpOffer2 = XRP (500);

            env.fund (r + f + xrpOffer, bob);
            env (offer (bob, usdOffer2, xrpOffer2),   ter(tesSUCCESS));
            env.fund (r + f, alice);
            env (trust (alice, usdOffer),             ter(tesSUCCESS));
            env (pay (gw, alice, usdOffer),           ter(tesSUCCESS));
            env (offer (alice, xrpOffer, usdOffer),   ter(tesSUCCESS));

            env.require (
                balance (alice, r - f + xrpOffer2),
                balance (alice, usdOffer2),
                owners (alice, 1),
                balance (bob, r + xrpOffer2),
                balance (bob, usdOffer2),
                owners (bob, 1));
        }

        {
            Env env {*this, features};
            auto const closeTime =
                fix1449Time () +
                    100 * env.closed ()->info ().closeTimeResolution;
            env.close (closeTime);

            env.fund (XRP (1000000), gw);

            auto const f = env.current ()->fees ().base;
            auto const r = reserve (env, 0);

            auto const usdOffer2 = USD (500);
            auto const xrpOffer2 = XRP (500);

            env.fund (r + f + xrpOffer, bob, carol);
            env (offer (bob, usdOffer2, xrpOffer2),    ter(tesSUCCESS));
            env (offer (carol, usdOffer, xrpOffer),    ter(tesSUCCESS));

            env.fund (r + f, alice);
            env (trust (alice, usdOffer),              ter(tesSUCCESS));
            env (pay (gw, alice, usdOffer),            ter(tesSUCCESS));
            env (offer (alice, xrpOffer, usdOffer),    ter(tesSUCCESS));

            env.require (
                balance (alice, r - f + xrpOffer),
                balance (alice, USD(0)),
                owners (alice, 1),
                balance (bob, r + xrpOffer2),
                balance (bob, usdOffer2),
                owners (bob, 1),
                balance (carol, r + xrpOffer2),
                balance (carol, usdOffer2),
                owners (carol, 2));
        }
    }

    static std::vector<std::shared_ptr<SLE const>>
    offersOnAccount (jtx::Env& env, jtx::Account account)
    {
        std::vector<std::shared_ptr<SLE const>> result;
        forEachItem (*env.current (), account,
            [&result](std::shared_ptr<SLE const> const& sle)
            {
                if (sle->getType() == ltOFFER)
                     result.push_back (sle);
            });
        return result;
    }

    void
    testFillModes (FeatureBitset features)
    {
        testcase ("Fill Modes");

        using namespace jtx;

        auto const startBalance = XRP (1000000);
        auto const gw = Account {"gateway"};
        auto const alice = Account {"alice"};
        auto const bob = Account {"bob"};
        auto const USD = gw["USD"];

        for (auto const tweakedFeatures :
            {features - fix1578, features | fix1578})
        {
            Env env {*this, tweakedFeatures};
            auto const closeTime =
                fix1449Time () +
                    100 * env.closed ()->info ().closeTimeResolution;
            env.close (closeTime);

            auto const f = env.current ()->fees ().base;

            env.fund (startBalance, gw, alice, bob);

            env (offer (bob, USD (500), XRP (500)),
                json (sfExpiration.fieldName, lastClose(env) + 1),
                ter(tesSUCCESS));

            env.close ();
            env.require (
                owners (bob, 1),
                offers (bob, 1));

            env (offer (bob, USD (500), XRP (500)), ter(tesSUCCESS));
            env.close();
            env.require (
                owners (bob, 2),
                offers (bob, 2));

            env (trust (alice, USD (1000)),         ter(tesSUCCESS));
            env (pay (gw, alice, USD (1000)),       ter(tesSUCCESS));

            {
                TER const killedCode {tweakedFeatures[fix1578] ?
                    TER {tecKILLED} : TER {tesSUCCESS}};
                env (offer (alice, XRP (1000), USD (1000)),
                    txflags (tfFillOrKill),         ter(killedCode));
            }
            env.require (
                balance (alice, startBalance - (f * 2)),
                balance (alice, USD (1000)),
                owners (alice, 1),
                offers (alice, 0),
                balance (bob, startBalance - (f * 2)),
                balance (bob, USD (none)),
                owners (bob, 1),
                offers (bob, 1));

            env (offer (alice, XRP (500), USD (500)),
                txflags (tfFillOrKill),              ter(tesSUCCESS));

            env.require (
                balance (alice, startBalance - (f * 3) + XRP (500)),
                balance (alice, USD (500)),
                owners (alice, 1),
                offers (alice, 0),
                balance (bob, startBalance - (f * 2) - XRP (500)),
                balance (bob, USD (500)),
                owners (bob, 1),
                offers (bob, 0));
        }

        {
            Env env {*this, features};
            auto const closeTime =
                fix1449Time () +
                    100 * env.closed ()->info ().closeTimeResolution;
            env.close (closeTime);

            auto const f = env.current ()->fees ().base;

            env.fund (startBalance, gw, alice, bob);

            env (trust (alice, USD (1000)),                 ter(tesSUCCESS));
            env (pay (gw, alice, USD (1000)),               ter(tesSUCCESS));

            env (offer (alice, XRP (1000), USD (1000)),
                txflags (tfImmediateOrCancel),              ter(tesSUCCESS));

            env.require (
                balance (alice, startBalance - f - f),
                balance (alice, USD (1000)),
                owners (alice, 1),
                offers (alice, 0));

            env (offer (bob, USD (50), XRP (50)),           ter(tesSUCCESS));
            env (offer (alice, XRP (1000), USD (1000)),
                txflags (tfImmediateOrCancel),              ter(tesSUCCESS));

            env.require (
                balance (alice, startBalance - f - f - f + XRP (50)),
                balance (alice, USD (950)),
                owners (alice, 1),
                offers (alice, 0),
                balance (bob, startBalance - f - XRP (50)),
                balance (bob, USD (50)),
                owners (bob, 1),
                offers (bob, 0));

            env (offer (bob, USD (50), XRP (50)),           ter(tesSUCCESS));
            env (offer (alice, XRP (50), USD (50)),
                txflags (tfImmediateOrCancel),              ter(tesSUCCESS));

            env.require (
                balance (alice, startBalance - f - f - f - f + XRP (100)),
                balance (alice, USD (900)),
                owners (alice, 1),
                offers (alice, 0),
                balance (bob, startBalance - f - f - XRP (100)),
                balance (bob, USD (100)),
                owners (bob, 1),
                offers (bob, 0));
        }

        {
            Env env (*this, features);
            auto const closeTime =
                fix1449Time () +
                    100 * env.closed ()->info ().closeTimeResolution;
            env.close (closeTime);

            env.fund (startBalance, gw, alice, bob);
            env.close();

            env (trust (bob, USD(1000)));
            env.close();

            env (pay (gw, bob, USD(1000)));
            env.close();

            env (offer (alice, USD(1000), XRP(2000)));
            env.close();

            auto const aliceOffers = offersOnAccount (env, alice);
            BEAST_EXPECT (aliceOffers.size() == 1);
            for (auto offerPtr : aliceOffers)
            {
                auto const& offer = *offerPtr;
                BEAST_EXPECT (offer[sfTakerGets] == XRP (2000));
                BEAST_EXPECT (offer[sfTakerPays] == USD (1000));
            }

            env (offer (bob, XRP(2000), USD(1000), tfPassive));
            env.close();
            env.require (offers (alice, 1));

            auto const bobOffers = offersOnAccount (env, bob);
            BEAST_EXPECT (bobOffers.size() == 1);
            for (auto offerPtr : bobOffers)
            {
                auto const& offer = *offerPtr;
                BEAST_EXPECT (offer[sfTakerGets] == USD (1000));
                BEAST_EXPECT (offer[sfTakerPays] == XRP (2000));
            }

            env (offer (gw, XRP(2000), USD(1000)));
            env.close();
            env.require (offers (alice, 0));
            env.require (offers (gw, 0));
            env.require (offers (bob, 1));

            env (offer (gw, USD(1000), XRP(2000)));
            env.close();
            env.require (offers (bob, 0));
            env.require (offers (gw, 0));
        }

        {
            Env env (*this, features);
            auto const closeTime =
                fix1449Time () +
                    100 * env.closed ()->info ().closeTimeResolution;
            env.close (closeTime);

            env.fund (startBalance, gw, "alice", "bob");
            env.close();

            env (trust ("bob", USD(1000)));
            env.close();

            env (pay (gw, "bob", USD(1000)));
            env (offer ("alice", USD(500), XRP(1001)));
            env.close();

            env (offer ("alice", USD(500), XRP(1000)));
            env.close();

            auto const aliceOffers = offersOnAccount (env, "alice");
            BEAST_EXPECT (aliceOffers.size() == 2);

            env (offer ("bob", XRP(2000), USD(1000), tfPassive));
            env.close();
            env.require (offers ("alice", 1));

            auto const bobOffers = offersOnAccount (env, "bob");
            BEAST_EXPECT (bobOffers.size() == 1);
            for (auto offerPtr : bobOffers)
            {
                auto const& offer = *offerPtr;
                BEAST_EXPECT (offer[sfTakerGets] == USD (499.5));
                BEAST_EXPECT (offer[sfTakerPays] == XRP (999));
            }
        }
    }

    void
    testMalformed(FeatureBitset features)
    {
        testcase ("Malformed Detection");

        using namespace jtx;

        auto const startBalance = XRP(1000000);
        auto const gw = Account {"gateway"};
        auto const alice = Account {"alice"};
        auto const USD = gw["USD"];

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        env.fund (startBalance, gw, alice);

        env (offer (alice, USD (1000), XRP (1000)),
            txflags (tfImmediateOrCancel + 1),            ter(temINVALID_FLAG));
        env.require (
            balance (alice, startBalance),
            owners (alice, 0),
            offers (alice, 0));

        env (offer (alice, USD (1000), XRP (1000)),
            txflags (tfImmediateOrCancel | tfFillOrKill), ter(temINVALID_FLAG));
        env.require (
            balance (alice, startBalance),
            owners (alice, 0),
            offers (alice, 0));

        {
            env (offer (alice, XRP (1000), XRP (1000)),   ter(temBAD_OFFER));
            env.require (
                owners (alice, 0),
                offers (alice, 0));

            env (trust (alice, USD (1000)),               ter(tesSUCCESS));
            env (pay (gw, alice, USD (1000)),             ter(tesSUCCESS));
            env (offer (alice, USD (1000), USD (1000)),   ter(temREDUNDANT));
            env.require (
                owners (alice, 1),
                offers (alice, 0));
        }

        {
            env (offer (alice, -USD (1000), XRP (1000)),  ter(temBAD_OFFER));
            env.require (
                owners (alice, 1),
                offers (alice, 0));

            env (offer (alice, USD (1000), -XRP (1000)),  ter(temBAD_OFFER));
            env.require (
                owners (alice, 1),
                offers (alice, 0));
        }

        {
            env (offer (alice, USD (1000), XRP (1000)),
                json (sfExpiration.fieldName, std::uint32_t (0)),
                ter(temBAD_EXPIRATION));
            env.require (
                owners (alice, 1),
                offers (alice, 0));
        }

        {
            env (offer (alice, USD (1000), XRP (1000)),
                json (jss::OfferSequence, std::uint32_t (0)),
                ter(temBAD_SEQUENCE));
            env.require (
                owners (alice, 1),
                offers (alice, 0));
        }

        {
            auto const BAD = IOU(gw, badCurrency());

            env (offer (alice, XRP (1000), BAD (1000)),   ter(temBAD_CURRENCY));
            env.require (
                owners (alice, 1),
                offers (alice, 0));
        }
    }

    void
    testExpiration(FeatureBitset features)
    {
        testcase ("Offer Expiration");

        using namespace jtx;

        auto const gw = Account {"gateway"};
        auto const alice = Account {"alice"};
        auto const bob = Account {"bob"};
        auto const USD = gw["USD"];

        auto const startBalance = XRP (1000000);
        auto const usdOffer = USD (1000);
        auto const xrpOffer = XRP (1000);

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        env.fund (startBalance, gw, alice, bob);
        env.close();

        auto const f = env.current ()->fees ().base;

        env (trust (alice, usdOffer),             ter(tesSUCCESS));
        env (pay (gw, alice, usdOffer),           ter(tesSUCCESS));
        env.close();
        env.require (
            balance (alice, startBalance - f),
            balance (alice, usdOffer),
            offers (alice, 0),
            owners (alice, 1));

        bool const featPreauth {features[featureDepositPreauth]};

        env (offer (alice, xrpOffer, usdOffer),
            json (sfExpiration.fieldName, lastClose(env)),
            ter (featPreauth ? TER {tecEXPIRED} : TER {tesSUCCESS}));

        env.require (
            balance (alice, startBalance - f - f),
            balance (alice, usdOffer),
            offers (alice, 0),
            owners (alice, 1));
        env.close();

        env (offer (alice, xrpOffer, usdOffer),
            json (sfExpiration.fieldName, lastClose(env) + 1),
            ter(tesSUCCESS));
        env.require (
            balance (alice, startBalance - f - f - f),
            balance (alice, usdOffer),
            offers (alice, 1),
            owners (alice, 2));

        env.close ();
        env.require (
            balance (alice, startBalance - f - f - f),
            balance (alice, usdOffer),
            offers (alice, 1),
            owners (alice, 2));

        env (offer (bob, usdOffer, xrpOffer),     ter(tesSUCCESS));
        env.require (
            balance (alice, startBalance - f - f - f),
            balance (alice, usdOffer),
            offers (alice, 0),
            owners (alice, 1),
            balance (bob, startBalance - f),
            balance (bob, USD (none)),
            offers (bob, 1),
            owners (bob, 1));
    }

    void
    testUnfundedCross(FeatureBitset features)
    {
        testcase ("Unfunded Crossing");

        using namespace jtx;

        auto const gw = Account {"gateway"};
        auto const USD = gw["USD"];

        auto const usdOffer = USD (1000);
        auto const xrpOffer = XRP (1000);

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        env.fund (XRP(1000000), gw);

        auto const f = env.current ()->fees ().base;

        env.fund (reserve (env, 0), "alice");
        env (offer ("alice", usdOffer, xrpOffer),     ter(tecUNFUNDED_OFFER));
        env.require (
            balance ("alice", reserve (env, 0) - f),
            owners ("alice", 0));

        env.fund (reserve (env, 0) + f, "bob");
        env (offer ("bob", usdOffer, xrpOffer),       ter(tecUNFUNDED_OFFER));
        env.require (
            balance ("bob", reserve (env, 0)),
            owners ("bob", 0));

        env.fund (reserve (env, 0) + f + XRP(1), "carol");
        env (offer ("carol", usdOffer, xrpOffer), ter(tecINSUF_RESERVE_OFFER));
        env.require (
            balance ("carol", reserve (env, 0) + XRP(1)),
            owners ("carol", 0));

        env.fund (reserve (env, 1) + f, "dan");
        env (offer ("dan", usdOffer, xrpOffer),       ter(tesSUCCESS));
        env.require (
            balance ("dan", reserve (env, 1)),
            owners ("dan", 1));

        env.fund (reserve (env, 1) + f + xrpOffer, "eve");
        env (offer ("eve", usdOffer, xrpOffer),       ter(tesSUCCESS));
        env.require (
            balance ("eve", reserve (env, 1) + xrpOffer),
            owners ("eve", 1));
    }

    void
    testSelfCross(bool use_partner, FeatureBitset features)
    {
        testcase (std::string("Self-crossing") +
            (use_partner ? ", with partner account" : ""));

        using namespace jtx;

        auto const gw = Account {"gateway"};
        auto const partner = Account {"partner"};
        auto const USD = gw["USD"];
        auto const BTC = gw["BTC"];

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        env.fund (XRP (10000), gw);
        if (use_partner)
        {
            env.fund (XRP (10000), partner);
            env (trust (partner, USD (100)));
            env (trust (partner, BTC (500)));
            env (pay (gw, partner, USD(100)));
            env (pay (gw, partner, BTC(500)));
        }
        auto const& account_to_test = use_partner ? partner : gw;

        env.close();
        env.require (offers (account_to_test, 0));

        env (offer (account_to_test, BTC (250), XRP (1000)));
        env.require (offers(account_to_test, 1));

        BEAST_EXPECT(isOffer(env, account_to_test, BTC(250), XRP(1000)));

        auto const secondLegSeq = env.seq(account_to_test);
        env (offer (account_to_test, XRP(1000), USD (50)));
        env.require (offers (account_to_test, 2));

        BEAST_EXPECT(isOffer(env, account_to_test, XRP(1000), USD(50)));

        env (offer (account_to_test, USD (50), BTC (250)));

        auto jrr = getBookOffers(env, USD, BTC);
        BEAST_EXPECT(jrr[jss::offers].isArray());
        BEAST_EXPECT(jrr[jss::offers].size() == 0);

        jrr = getBookOffers(env, BTC, XRP);
        BEAST_EXPECT(jrr[jss::offers].isArray());
        BEAST_EXPECT(jrr[jss::offers].size() == 0);

        {
            auto acctOffers = offersOnAccount (env, account_to_test);
            bool const noStaleOffers {
                features[featureFlowCross] ||
                features[fixTakerDryOfferRemoval]};

            BEAST_EXPECT(acctOffers.size() == (noStaleOffers ? 0 : 1));
            for (auto const& offerPtr : acctOffers)
            {
                auto const& offer = *offerPtr;
                BEAST_EXPECT (offer[sfLedgerEntryType] == ltOFFER);
                BEAST_EXPECT (offer[sfTakerGets] == USD (0));
                BEAST_EXPECT (offer[sfTakerPays] == XRP (0));
            }
        }

        Json::Value cancelOffer;
        cancelOffer[jss::Account] = account_to_test.human();
        cancelOffer[jss::OfferSequence] = secondLegSeq;
        cancelOffer[jss::TransactionType] = jss::OfferCancel;
        env (cancelOffer);
        env.require (offers (account_to_test, 0));

        env (offer (account_to_test, BTC (250), USD (50)));
        env.require (offers (account_to_test, 1));

        BEAST_EXPECT(isOffer(env, account_to_test, BTC(250), USD(50)));

        jrr = getBookOffers(env, USD, BTC);
        BEAST_EXPECT(jrr[jss::offers].isArray());
        BEAST_EXPECT(jrr[jss::offers].size() == 0);

        env (offer (account_to_test, USD (50), BTC (250)));
        env.require (offers (account_to_test, 1));

        jrr = getBookOffers(env, BTC, USD);
        BEAST_EXPECT(jrr[jss::offers].isArray());
        BEAST_EXPECT(jrr[jss::offers].size() == 0);

        BEAST_EXPECT(isOffer(env, account_to_test, USD(50), BTC(250)));
    }

    void
    testNegativeBalance(FeatureBitset features)
    {
        testcase ("Negative Balance");

        using namespace jtx;

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        auto const gw = Account {"gateway"};
        auto const alice = Account {"alice"};
        auto const bob = Account {"bob"};
        auto const USD = gw["USD"];
        auto const BTC = gw["BTC"];

        auto const gw_initial_balance = drops (1149999730);
        auto const alice_initial_balance = drops (499946999680);
        auto const bob_initial_balance = drops (10199999920);
        auto const small_amount =
            STAmount { bob["USD"].issue(), UINT64_C(2710505431213761), -33};

        env.fund (gw_initial_balance, gw);
        env.fund (alice_initial_balance, alice);
        env.fund (bob_initial_balance, bob);

        env (rate (gw, 1.005));

        env (trust (alice, USD (500)));
        env (trust (bob, USD (50)));
        env (trust (gw, alice["USD"] (100)));

        env (pay (gw, alice, alice["USD"] (50)));
        env (pay (gw, bob, small_amount));

        env (offer (alice, USD (50), XRP (150000)));

        env (pay (alice, gw, USD (100)));

        env (trust (gw, alice["USD"] (0)));

        auto jrr = ledgerEntryState (env, alice, gw, "USD");
        BEAST_EXPECT(jrr[jss::node][sfBalance.fieldName][jss::value] == "50");

        jrr = ledgerEntryState (env, bob, gw, "USD");
        BEAST_EXPECT(
            jrr[jss::node][sfBalance.fieldName][jss::value] ==
                "-2710505431213761e-33");

        env (offer (bob, XRP (2000), USD (1)));

        auto const crossingDelta = features[featureFlowCross] ? drops (1) : drops (0);

        jrr = ledgerEntryState (env, alice, gw, "USD");
        BEAST_EXPECT(jrr[jss::node][sfBalance.fieldName][jss::value] == "50");
        BEAST_EXPECT(env.balance (alice, xrpIssue()) ==
            alice_initial_balance -
                env.current ()->fees ().base * 3 - crossingDelta
        );

        jrr = ledgerEntryState (env, bob, gw, "USD");
        BEAST_EXPECT( jrr[jss::node][sfBalance.fieldName][jss::value] == "0");
        BEAST_EXPECT(env.balance (bob, xrpIssue()) ==
            bob_initial_balance -
                env.current ()->fees ().base * 2 + crossingDelta
        );
    }

    void
    testOfferCrossWithXRP(bool reverse_order, FeatureBitset features)
    {
        testcase (std::string("Offer Crossing with XRP, ") +
                (reverse_order ? "Reverse" : "Normal") +
                " order");

        using namespace jtx;

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        auto const gw = Account {"gateway"};
        auto const alice = Account {"alice"};
        auto const bob = Account {"bob"};
        auto const USD = gw["USD"];

        env.fund (XRP (10000), gw, alice, bob);

        env (trust (alice, USD (1000)));
        env (trust (bob, USD (1000)));

        env (pay (gw, alice, alice["USD"] (500)));

        if (reverse_order)
            env (offer (bob, USD (1), XRP (4000)));

        env (offer (alice, XRP (150000), USD (50)));

        if (! reverse_order)
            env (offer (bob, USD (1), XRP (4000)));


        auto jrr = ledgerEntryState (env, bob, gw, "USD");
        BEAST_EXPECT(jrr[jss::node][sfBalance.fieldName][jss::value] == "-1");
        jrr = ledgerEntryRoot (env, bob);
        BEAST_EXPECT(
            jrr[jss::node][sfBalance.fieldName] ==
            std::to_string(
                XRP (10000).value ().mantissa () -
                XRP (reverse_order ? 4000 : 3000).value ().mantissa () -
                env.current ()->fees ().base * 2)
        );

        jrr = ledgerEntryState (env, alice, gw, "USD");
        BEAST_EXPECT(jrr[jss::node][sfBalance.fieldName][jss::value] == "-499");
        jrr = ledgerEntryRoot (env, alice);
        BEAST_EXPECT(
            jrr[jss::node][sfBalance.fieldName] ==
            std::to_string(
                XRP (10000).value ().mantissa( ) +
                XRP(reverse_order ? 4000 : 3000).value ().mantissa () -
                env.current ()->fees ().base * 2)
        );
    }

    void
    testOfferCrossWithLimitOverride(FeatureBitset features)
    {
        testcase ("Offer Crossing with Limit Override");

        using namespace jtx;

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        auto const gw = Account {"gateway"};
        auto const alice = Account {"alice"};
        auto const bob = Account {"bob"};
        auto const USD = gw["USD"];

        env.fund (XRP (100000), gw, alice, bob);

        env (trust (alice, USD (1000)));

        env (pay (gw, alice, alice["USD"] (500)));

        env (offer (alice, XRP (150000), USD (50)));
        env (offer (bob, USD (1), XRP (3000)));

        auto jrr = ledgerEntryState (env, bob, gw, "USD");
        BEAST_EXPECT(jrr[jss::node][sfBalance.fieldName][jss::value] == "-1");
        jrr = ledgerEntryRoot(env, bob);
        BEAST_EXPECT(
            jrr[jss::node][sfBalance.fieldName] ==
            std::to_string(
                XRP (100000).value ().mantissa () -
                XRP (3000).value ().mantissa () -
                env.current ()->fees ().base * 1)
        );

        jrr = ledgerEntryState (env, alice, gw, "USD");
        BEAST_EXPECT(jrr[jss::node][sfBalance.fieldName][jss::value] == "-499");
        jrr = ledgerEntryRoot (env, alice);
        BEAST_EXPECT(
            jrr[jss::node][sfBalance.fieldName] ==
            std::to_string(
                XRP (100000).value ().mantissa () +
                XRP (3000).value ().mantissa () -
                env.current ()->fees ().base * 2)
        );
    }

    void
    testOfferAcceptThenCancel(FeatureBitset features)
    {
        testcase ("Offer Accept then Cancel.");

        using namespace jtx;

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        auto const USD = env.master["USD"];

        auto const nextOfferSeq = env.seq (env.master);
        env (offer (env.master, XRP (500), USD (100)));
        env.close ();

        Json::Value cancelOffer;
        cancelOffer[jss::Account] = env.master.human();
        cancelOffer[jss::OfferSequence] = nextOfferSeq;
        cancelOffer[jss::TransactionType] = jss::OfferCancel;
        env (cancelOffer);
        BEAST_EXPECT(env.seq (env.master) == nextOfferSeq + 2);

        env.close();
        env.close();
        BEAST_EXPECT(env.seq (env.master) == nextOfferSeq + 2);
    }

    void
    testOfferCancelPastAndFuture(FeatureBitset features)
    {

        testcase ("Offer Cancel Past and Future Sequence.");

        using namespace jtx;

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        auto const alice = Account {"alice"};

        auto const nextOfferSeq = env.seq (env.master);
        env.fund (XRP (10000), alice);

        Json::Value cancelOffer;
        cancelOffer[jss::Account] = env.master.human();
        cancelOffer[jss::OfferSequence] = nextOfferSeq;
        cancelOffer[jss::TransactionType] = jss::OfferCancel;
        env (cancelOffer);

        cancelOffer[jss::OfferSequence] = env.seq (env.master);
        env (cancelOffer, ter(temBAD_SEQUENCE));

        cancelOffer[jss::OfferSequence] = env.seq (env.master) + 1;
        env (cancelOffer, ter(temBAD_SEQUENCE));

        env.close();
        env.close();
    }

    void
    testCurrencyConversionEntire(FeatureBitset features)
    {
        testcase ("Currency Conversion: Entire Offer");

        using namespace jtx;

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        auto const gw = Account {"gateway"};
        auto const alice = Account {"alice"};
        auto const bob = Account {"bob"};
        auto const USD = gw["USD"];

        env.fund (XRP (10000), gw, alice, bob);
        env.require (owners (bob, 0));

        env (trust (alice, USD (100)));
        env (trust (bob, USD (1000)));

        env.require (
            owners (alice, 1),
            owners (bob, 1));

        env (pay (gw, alice, alice["USD"] (100)));
        auto const bobOfferSeq = env.seq (bob);
        env (offer (bob, USD (100), XRP (500)));

        env.require (
            owners (alice, 1),
            owners (bob, 2));
        auto jro = ledgerEntryOffer (env, bob, bobOfferSeq);
        BEAST_EXPECT(
            jro[jss::node][jss::TakerGets] == XRP (500).value ().getText ());
        BEAST_EXPECT(
            jro[jss::node][jss::TakerPays] ==
            USD (100).value ().getJson (JsonOptions::none));

        env (pay (alice, alice, XRP (500)), sendmax (USD (100)));

        auto jrr = ledgerEntryState (env, alice, gw, "USD");
        BEAST_EXPECT(jrr[jss::node][sfBalance.fieldName][jss::value] == "0");
        jrr = ledgerEntryRoot (env, alice);
        BEAST_EXPECT(
            jrr[jss::node][sfBalance.fieldName] ==
            std::to_string(
                XRP (10000).value ().mantissa () +
                XRP (500).value ().mantissa () -
                env.current ()->fees ().base * 2)
        );

        jrr = ledgerEntryState (env, bob, gw, "USD");
        BEAST_EXPECT(jrr[jss::node][sfBalance.fieldName][jss::value] == "-100");

        jro = ledgerEntryOffer(env, bob, bobOfferSeq);
        BEAST_EXPECT(jro[jss::error] == "entryNotFound");

        env.require (
            owners (alice, 1),
            owners (bob, 1));
    }

    void
    testCurrencyConversionIntoDebt(FeatureBitset features)
    {
        testcase ("Currency Conversion: Offerer Into Debt");

        using namespace jtx;

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        auto const alice = Account {"alice"};
        auto const bob = Account {"bob"};
        auto const carol = Account {"carol"};

        env.fund (XRP (10000), alice, bob, carol);

        env (trust (alice, carol["EUR"] (2000)));
        env (trust (bob, alice["USD"] (100)));
        env (trust (carol, bob["EUR"] (1000)));

        auto const bobOfferSeq = env.seq (bob);
        env (offer (bob, alice["USD"] (50), carol["EUR"] (200)),
            ter(tecUNFUNDED_OFFER));

        env (offer (alice, carol["EUR"] (200), alice["USD"] (50)));

        auto jro = ledgerEntryOffer(env, bob, bobOfferSeq);
        BEAST_EXPECT(jro[jss::error] == "entryNotFound");
    }

    void
    testCurrencyConversionInParts(FeatureBitset features)
    {
        testcase ("Currency Conversion: In Parts");

        using namespace jtx;

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        auto const gw = Account {"gateway"};
        auto const alice = Account {"alice"};
        auto const bob = Account {"bob"};
        auto const USD = gw["USD"];

        env.fund (XRP (10000), gw, alice, bob);

        env (trust (alice, USD (200)));
        env (trust (bob, USD (1000)));

        env (pay (gw, alice, alice["USD"] (200)));

        auto const bobOfferSeq = env.seq (bob);
        env (offer (bob, USD (100), XRP (500)));

        env (pay (alice, alice, XRP (200)), sendmax (USD (100)));

        auto jro = ledgerEntryOffer (env, bob, bobOfferSeq);
        BEAST_EXPECT(
            jro[jss::node][jss::TakerGets] == XRP (300).value ().getText ());
        BEAST_EXPECT(
            jro[jss::node][jss::TakerPays] ==
            USD (60).value ().getJson (JsonOptions::none));

        auto jrr = ledgerEntryState (env, alice, gw, "USD");
        BEAST_EXPECT(jrr[jss::node][sfBalance.fieldName][jss::value] == "-160");
        jrr = ledgerEntryRoot (env, alice);
        BEAST_EXPECT(
            jrr[jss::node][sfBalance.fieldName] ==
            std::to_string(
                XRP (10000).value ().mantissa () +
                XRP (200).value ().mantissa () -
                env.current ()->fees ().base * 2)
        );

        jrr = ledgerEntryState (env, bob, gw, "USD");
        BEAST_EXPECT(jrr[jss::node][sfBalance.fieldName][jss::value] == "-40");

        env (pay (alice, alice, XRP (600)), sendmax (USD (100)),
            ter(tecPATH_PARTIAL));

        env (pay (alice, alice, XRP (600)), sendmax (USD (100)),
            txflags (tfPartialPayment));

        jro = ledgerEntryOffer (env, bob, bobOfferSeq);
        BEAST_EXPECT(jro[jss::error] == "entryNotFound");

        jrr = ledgerEntryState (env, alice, gw, "USD");
        BEAST_EXPECT(jrr[jss::node][sfBalance.fieldName][jss::value] == "-100");
        jrr = ledgerEntryRoot (env, alice);
        BEAST_EXPECT(
            jrr[jss::node][sfBalance.fieldName] ==
            std::to_string(
                XRP (10000).value ().mantissa () +
                XRP (200).value ().mantissa () +
                XRP (300).value ().mantissa () -
                env.current ()->fees ().base * 4)
        );

        jrr = ledgerEntryState (env, bob, gw, "USD");
        BEAST_EXPECT(jrr[jss::node][sfBalance.fieldName][jss::value] == "-100");
    }

    void
    testCrossCurrencyStartXRP(FeatureBitset features)
    {
        testcase ("Cross Currency Payment: Start with XRP");

        using namespace jtx;

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        auto const gw = Account {"gateway"};
        auto const alice = Account {"alice"};
        auto const bob = Account {"bob"};
        auto const carol = Account {"carol"};
        auto const USD = gw["USD"];

        env.fund (XRP (10000), gw, alice, bob, carol);

        env (trust (carol, USD (1000)));
        env (trust (bob, USD (2000)));

        env (pay (gw, carol, carol["USD"] (500)));

        auto const carolOfferSeq = env.seq (carol);
        env (offer (carol, XRP (500), USD (50)));

        env (pay (alice, bob, USD (25)), sendmax (XRP (333)));

        auto jrr = ledgerEntryState (env, bob, gw, "USD");
        BEAST_EXPECT(jrr[jss::node][sfBalance.fieldName][jss::value] == "-25");

        jrr = ledgerEntryState (env, carol, gw, "USD");
        BEAST_EXPECT(jrr[jss::node][sfBalance.fieldName][jss::value] == "-475");

        auto jro = ledgerEntryOffer (env, carol, carolOfferSeq);
        BEAST_EXPECT(
            jro[jss::node][jss::TakerGets] ==
            USD (25).value ().getJson (JsonOptions::none));
        BEAST_EXPECT(
            jro[jss::node][jss::TakerPays] == XRP (250).value ().getText ());
    }

    void
    testCrossCurrencyEndXRP(FeatureBitset features)
    {
        testcase ("Cross Currency Payment: End with XRP");

        using namespace jtx;

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        auto const gw = Account {"gateway"};
        auto const alice = Account {"alice"};
        auto const bob = Account {"bob"};
        auto const carol = Account {"carol"};
        auto const USD = gw["USD"];

        env.fund (XRP (10000), gw, alice, bob, carol);

        env (trust (alice, USD (1000)));
        env (trust (carol, USD (2000)));

        env (pay (gw, alice, alice["USD"] (500)));

        auto const carolOfferSeq = env.seq (carol);
        env (offer (carol, USD (50), XRP (500)));

        env (pay (alice, bob, XRP (250)), sendmax (USD (333)));

        auto jrr = ledgerEntryState (env, alice, gw, "USD");
        BEAST_EXPECT(jrr[jss::node][sfBalance.fieldName][jss::value] == "-475");

        jrr = ledgerEntryState (env, carol, gw, "USD");
        BEAST_EXPECT(jrr[jss::node][sfBalance.fieldName][jss::value] == "-25");

        jrr = ledgerEntryRoot (env, bob);
        BEAST_EXPECT(
            jrr[jss::node][sfBalance.fieldName] ==
            std::to_string(
                XRP (10000).value ().mantissa () +
                XRP (250).value ().mantissa ())
        );

        auto jro = ledgerEntryOffer (env, carol, carolOfferSeq);
        BEAST_EXPECT(
            jro[jss::node][jss::TakerGets] == XRP (250).value ().getText ());
        BEAST_EXPECT(
            jro[jss::node][jss::TakerPays] ==
            USD (25).value ().getJson (JsonOptions::none));
    }

    void
    testCrossCurrencyBridged(FeatureBitset features)
    {
        testcase ("Cross Currency Payment: Bridged");

        using namespace jtx;

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        auto const gw1 = Account {"gateway_1"};
        auto const gw2 = Account {"gateway_2"};
        auto const alice = Account {"alice"};
        auto const bob = Account {"bob"};
        auto const carol = Account {"carol"};
        auto const dan = Account {"dan"};
        auto const USD = gw1["USD"];
        auto const EUR = gw2["EUR"];

        env.fund (XRP (10000), gw1, gw2, alice, bob, carol, dan);

        env (trust (alice, USD (1000)));
        env (trust (bob, EUR (1000)));
        env (trust (carol, USD (1000)));
        env (trust (dan, EUR (1000)));

        env (pay (gw1, alice, alice["USD"] (500)));
        env (pay (gw2, dan, dan["EUR"] (400)));

        auto const carolOfferSeq = env.seq (carol);
        env (offer (carol, USD (50), XRP (500)));

        auto const danOfferSeq = env.seq (dan);
        env (offer (dan, XRP (500), EUR (50)));

        Json::Value jtp {Json::arrayValue};
        jtp[0u][0u][jss::currency] = "XRP";
        env (pay (alice, bob, EUR (30)),
            json (jss::Paths, jtp),
            sendmax (USD (333)));

        auto jrr = ledgerEntryState (env, alice, gw1, "USD");
        BEAST_EXPECT(jrr[jss::node][sfBalance.fieldName][jss::value] == "470");

        jrr = ledgerEntryState (env, bob, gw2, "EUR");
        BEAST_EXPECT(jrr[jss::node][sfBalance.fieldName][jss::value] == "-30");

        jrr = ledgerEntryState (env, carol, gw1, "USD");
        BEAST_EXPECT(jrr[jss::node][sfBalance.fieldName][jss::value] == "-30");

        jrr = ledgerEntryState (env, dan, gw2, "EUR");
        BEAST_EXPECT(jrr[jss::node][sfBalance.fieldName][jss::value] == "-370");

        auto jro = ledgerEntryOffer (env, carol, carolOfferSeq);
        BEAST_EXPECT(
            jro[jss::node][jss::TakerGets] == XRP (200).value ().getText ());
        BEAST_EXPECT(
            jro[jss::node][jss::TakerPays] ==
            USD (20).value ().getJson (JsonOptions::none));

        jro = ledgerEntryOffer (env, dan, danOfferSeq);
        BEAST_EXPECT(
            jro[jss::node][jss::TakerGets] ==
            gw2["EUR"] (20).value ().getJson (JsonOptions::none));
        BEAST_EXPECT(
            jro[jss::node][jss::TakerPays] == XRP (200).value ().getText ());
    }

    void
    testBridgedSecondLegDry(FeatureBitset features)
    {
        testcase ("Auto Bridged Second Leg Dry");

        using namespace jtx;
        Env env(*this, features);
        auto const closeTime =
            fix1449Time() +
                100 * env.closed()->info().closeTimeResolution;
        env.close (closeTime);

        Account const alice {"alice"};
        Account const bob {"bob"};
        Account const carol {"carol"};
        Account const gw {"gateway"};
        auto const USD = gw["USD"];
        auto const EUR = gw["EUR"];

        env.fund(XRP(100000000), alice, bob, carol, gw);

        env.trust(USD(10), alice);
        env.close();
        env(pay(gw, alice, USD(10)));
        env.trust(USD(10), carol);
        env.close();
        env(pay(gw, carol, USD(3)));

        env (offer (alice, EUR(2), XRP(1)));
        env (offer (alice, EUR(2), XRP(1)));

        env (offer (alice, XRP(1), USD(4)));
        env (offer (carol, XRP(1), USD(3)));
        env.close();

        env.trust(EUR(10), bob);
        env.close();
        env(pay(gw, bob, EUR(10)));
        env.close();
        env(offer(bob, USD(10), EUR(10)));
        env.close();

        env.require (balance(bob, USD(7)));
        env.require (balance(bob, EUR(6)));
        env.require (offers (bob, 1));
        env.require (owners (bob, 3));

        env.require (balance(alice, USD(6)));
        env.require (balance(alice, EUR(4)));
        env.require (offers(alice, 0));
        env.require (owners(alice, 2));

        env.require (balance(carol, USD(0)));
        env.require (balance(carol, EUR(none)));
        int const emptyOfferCount {
            features[featureFlowCross] ||
            features[fixTakerDryOfferRemoval] ? 0 : 1};

        env.require (offers(carol, 0 + emptyOfferCount));
        env.require (owners(carol, 1 + emptyOfferCount));
    }

    void
    testOfferFeesConsumeFunds(FeatureBitset features)
    {
        testcase ("Offer Fees Consume Funds");

        using namespace jtx;

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        auto const gw1 = Account {"gateway_1"};
        auto const gw2 = Account {"gateway_2"};
        auto const gw3 = Account {"gateway_3"};
        auto const alice = Account {"alice"};
        auto const bob = Account {"bob"};
        auto const USD1 = gw1["USD"];
        auto const USD2 = gw2["USD"];
        auto const USD3 = gw3["USD"];

        auto const starting_xrp = XRP (100) +
            env.current ()->fees ().accountReserve (3) +
            env.current ()->fees ().base * 4;

        env.fund (starting_xrp, gw1, gw2, gw3, alice, bob);

        env (trust (alice, USD1 (1000)));
        env (trust (alice, USD2 (1000)));
        env (trust (alice, USD3 (1000)));
        env (trust (bob, USD1 (1000)));
        env (trust (bob, USD2 (1000)));

        env (pay (gw1, bob, bob["USD"] (500)));

        env (offer (bob, XRP (200), USD1 (200)));
        env (offer (alice, USD1 (200), XRP (200)));

        auto jrr = ledgerEntryState (env, alice, gw1, "USD");
        BEAST_EXPECT(jrr[jss::node][sfBalance.fieldName][jss::value] == "100");
        jrr = ledgerEntryRoot (env, alice);
        BEAST_EXPECT(
            jrr[jss::node][sfBalance.fieldName] == XRP (350).value ().getText ()
        );

        jrr = ledgerEntryState (env, bob, gw1, "USD");
        BEAST_EXPECT(jrr[jss::node][sfBalance.fieldName][jss::value] == "-400");
    }

    void
    testOfferCreateThenCross(FeatureBitset features)
    {
        testcase ("Offer Create, then Cross");

        using namespace jtx;

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        auto const gw = Account {"gateway"};
        auto const alice = Account {"alice"};
        auto const bob = Account {"bob"};
        auto const USD = gw["USD"];

        env.fund (XRP (10000), gw, alice, bob);

        env (rate (gw, 1.005));

        env (trust (alice, USD (1000)));
        env (trust (bob, USD (1000)));
        env (trust (gw, alice["USD"] (50)));

        env (pay (gw, bob, bob["USD"] (1)));
        env (pay (alice, gw, USD (50)));

        env (trust (gw, alice["USD"] (0)));

        env (offer (alice, USD (50), XRP (150000)));
        env (offer (bob, XRP (100), USD (0.1)));

        auto jrr = ledgerEntryState (env, alice, gw, "USD");
        BEAST_EXPECT(jrr[jss::node][sfBalance.fieldName][jss::value] == "49.96666666666667");
        jrr = ledgerEntryState (env, bob, gw, "USD");
        BEAST_EXPECT(jrr[jss::node][sfBalance.fieldName][jss::value] == "-0.966500000033334");
    }

    void
    testSellFlagBasic(FeatureBitset features)
    {
        testcase ("Offer tfSell: Basic Sell");

        using namespace jtx;

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        auto const gw = Account {"gateway"};
        auto const alice = Account {"alice"};
        auto const bob = Account {"bob"};
        auto const USD = gw["USD"];

        auto const starting_xrp = XRP (100) +
            env.current ()->fees ().accountReserve (1) +
            env.current ()->fees ().base * 2;

        env.fund (starting_xrp, gw, alice, bob);

        env (trust (alice, USD (1000)));
        env (trust (bob, USD (1000)));

        env (pay (gw, bob, bob["USD"] (500)));

        env (offer (bob, XRP (200), USD (200)), json(jss::Flags, tfSell));
        env (offer (alice, USD (200), XRP (200)), json(jss::Flags, tfSell));

        auto jrr = ledgerEntryState (env, alice, gw, "USD");
        BEAST_EXPECT(jrr[jss::node][sfBalance.fieldName][jss::value] == "-100");
        jrr = ledgerEntryRoot (env, alice);
        BEAST_EXPECT(
            jrr[jss::node][sfBalance.fieldName] == XRP (250).value ().getText ()
        );

        jrr = ledgerEntryState (env, bob, gw, "USD");
        BEAST_EXPECT(jrr[jss::node][sfBalance.fieldName][jss::value] == "-400");
    }

    void
    testSellFlagExceedLimit(FeatureBitset features)
    {
        testcase ("Offer tfSell: 2x Sell Exceed Limit");

        using namespace jtx;

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        auto const gw = Account {"gateway"};
        auto const alice = Account {"alice"};
        auto const bob = Account {"bob"};
        auto const USD = gw["USD"];

        auto const starting_xrp = XRP (100) +
            env.current ()->fees ().accountReserve (1) +
            env.current ()->fees ().base * 2;

        env.fund (starting_xrp, gw, alice, bob);

        env (trust (alice, USD (150)));
        env (trust (bob, USD (1000)));

        env (pay (gw, bob, bob["USD"] (500)));

        env (offer (bob, XRP (100), USD (200)));
        env (offer (alice, USD (100), XRP (100)), json(jss::Flags, tfSell));

        auto jrr = ledgerEntryState (env, alice, gw, "USD");
        BEAST_EXPECT(jrr[jss::node][sfBalance.fieldName][jss::value] == "-200");
        jrr = ledgerEntryRoot (env, alice);
        BEAST_EXPECT(
            jrr[jss::node][sfBalance.fieldName] == XRP (250).value ().getText ()
        );

        jrr = ledgerEntryState (env, bob, gw, "USD");
        BEAST_EXPECT(jrr[jss::node][sfBalance.fieldName][jss::value] == "-300");
    }

    void
    testGatewayCrossCurrency(FeatureBitset features)
    {
        testcase ("Client Issue #535: Gateway Cross Currency");

        using namespace jtx;

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        auto const gw = Account {"gateway"};
        auto const alice = Account {"alice"};
        auto const bob = Account {"bob"};
        auto const XTS = gw["XTS"];
        auto const XXX = gw["XXX"];

        auto const starting_xrp = XRP (100.1) +
            env.current ()->fees ().accountReserve (1) +
            env.current ()->fees ().base * 2;

        env.fund (starting_xrp, gw, alice, bob);

        env (trust (alice, XTS (1000)));
        env (trust (alice, XXX (1000)));
        env (trust (bob, XTS (1000)));
        env (trust (bob, XXX (1000)));

        env (pay (gw, alice, alice["XTS"] (100)));
        env (pay (gw, alice, alice["XXX"] (100)));
        env (pay (gw, bob, bob["XTS"] (100)));
        env (pay (gw, bob, bob["XXX"] (100)));

        env (offer (alice, XTS (100), XXX (100)));

        auto wsc = makeWSClient(env.app().config());
        Json::Value payment;
        payment[jss::secret] = toBase58(generateSeed("bob"));
        payment[jss::id] = env.seq (bob);
        payment[jss::build_path] = true;
        payment[jss::tx_json] = pay (bob, bob, bob["XXX"] (1));
        payment[jss::tx_json][jss::Sequence] =
            env.current ()->read (
                keylet::account (bob.id ()))->getFieldU32 (sfSequence);
        payment[jss::tx_json][jss::Fee] =
            std::to_string( env.current ()->fees ().base);
        payment[jss::tx_json][jss::SendMax] =
            bob ["XTS"] (1.5).value ().getJson (JsonOptions::none);
        auto jrr = wsc->invoke("submit", payment);
        BEAST_EXPECT(jrr[jss::status] == "success");
        BEAST_EXPECT(jrr[jss::result][jss::engine_result] == "tesSUCCESS");
        if (wsc->version() == 2)
        {
            BEAST_EXPECT(jrr.isMember(jss::jsonrpc) && jrr[jss::jsonrpc] == "2.0");
            BEAST_EXPECT(jrr.isMember(jss::ripplerpc) && jrr[jss::ripplerpc] == "2.0");
            BEAST_EXPECT(jrr.isMember(jss::id) && jrr[jss::id] == 5);
        }

        jrr = ledgerEntryState (env, alice, gw, "XTS");
        BEAST_EXPECT(jrr[jss::node][sfBalance.fieldName][jss::value] == "-101");
        jrr = ledgerEntryState (env, alice, gw, "XXX");
        BEAST_EXPECT(jrr[jss::node][sfBalance.fieldName][jss::value] == "-99");

        jrr = ledgerEntryState (env, bob, gw, "XTS");
        BEAST_EXPECT(jrr[jss::node][sfBalance.fieldName][jss::value] == "-99");
        jrr = ledgerEntryState (env, bob, gw, "XXX");
        BEAST_EXPECT(jrr[jss::node][sfBalance.fieldName][jss::value] == "-101");
    }

    void
    verifyDefaultTrustline (jtx::Env& env,
        jtx::Account const& account, jtx::PrettyAmount const& expectBalance)
    {
        auto const sleTrust =
            env.le (keylet::line(account.id(), expectBalance.value().issue()));
        BEAST_EXPECT (sleTrust);
        if (sleTrust)
        {
            Issue const issue = expectBalance.value().issue();
            bool const accountLow = account.id() < issue.account;

            STAmount low {issue};
            STAmount high {issue};

            low.setIssuer (accountLow ? account.id() : issue.account);
            high.setIssuer (accountLow ? issue.account : account.id());

            BEAST_EXPECT (sleTrust->getFieldAmount (sfLowLimit) == low);
            BEAST_EXPECT (sleTrust->getFieldAmount (sfHighLimit) == high);

            STAmount actualBalance {sleTrust->getFieldAmount (sfBalance)};
            if (! accountLow)
                actualBalance.negate();

            BEAST_EXPECT (actualBalance == expectBalance);
        }
    }

    void testPartialCross (FeatureBitset features)
    {
        testcase ("Partial Crossing");

        using namespace jtx;

        auto const gw = Account("gateway");
        auto const USD = gw["USD"];

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        env.fund (XRP(10000000), gw);

        auto const f = env.current ()->fees ().base;

        enum preTrustType {noPreTrust, gwPreTrust, acctPreTrust};
        struct TestData
        {
            std::string account;       
            STAmount fundXrp;          
            int bookAmount;            
            preTrustType preTrust;     
            int offerAmount;           
            TER tec;                   
            STAmount spentXrp;         
            PrettyAmount balanceUsd;   
            int offers;                
            int owners;                
        };

        TestData const tests[]
        {
{"ann",             reserve (env, 0) + 0*f,       1,   noPreTrust,     1000,      tecUNFUNDED_OFFER,             f, USD(      0),      0,     0}, 
{"bev",             reserve (env, 0) + 1*f,       1,   noPreTrust,     1000,      tecUNFUNDED_OFFER,             f, USD(      0),      0,     0}, 
{"cam",             reserve (env, 0) + 2*f,       0,   noPreTrust,     1000, tecINSUF_RESERVE_OFFER,             f, USD(      0),      0,     0}, 
{"deb",             reserve (env, 0) + 2*f,       1,   noPreTrust,     1000,             tesSUCCESS,           2*f, USD(0.00001),      0,     1}, 
{"eve",             reserve (env, 1) + 0*f,       0,   noPreTrust,     1000,             tesSUCCESS,             f, USD(      0),      1,     1}, 
{"flo",             reserve (env, 1) + 0*f,       1,   noPreTrust,     1000,             tesSUCCESS, XRP(   1) + f, USD(      1),      0,     1},
{"gay",             reserve (env, 1) + 1*f,    1000,   noPreTrust,     1000,             tesSUCCESS, XRP(  50) + f, USD(     50),      0,     1},
{"hye", XRP(1000)                    + 1*f,    1000,   noPreTrust,     1000,             tesSUCCESS, XRP( 800) + f, USD(    800),      0,     1},
{"ivy", XRP(   1) + reserve (env, 1) + 1*f,       1,   noPreTrust,     1000,             tesSUCCESS, XRP(   1) + f, USD(      1),      0,     1},
{"joy", XRP(   1) + reserve (env, 2) + 1*f,       1,   noPreTrust,     1000,             tesSUCCESS, XRP(   1) + f, USD(      1),      1,     2},
{"kim", XRP( 900) + reserve (env, 2) + 1*f,     999,   noPreTrust,     1000,             tesSUCCESS, XRP( 999) + f, USD(    999),      0,     1},
{"liz", XRP( 998) + reserve (env, 0) + 1*f,     999,   noPreTrust,     1000,             tesSUCCESS, XRP( 998) + f, USD(    998),      0,     1},
{"meg", XRP( 998) + reserve (env, 1) + 1*f,     999,   noPreTrust,     1000,             tesSUCCESS, XRP( 999) + f, USD(    999),      0,     1},
{"nia", XRP( 998) + reserve (env, 2) + 1*f,     999,   noPreTrust,     1000,             tesSUCCESS, XRP( 999) + f, USD(    999),      1,     2},
{"ova", XRP( 999) + reserve (env, 0) + 1*f,    1000,   noPreTrust,     1000,             tesSUCCESS, XRP( 999) + f, USD(    999),      0,     1},
{"pam", XRP( 999) + reserve (env, 1) + 1*f,    1000,   noPreTrust,     1000,             tesSUCCESS, XRP(1000) + f, USD(   1000),      0,     1},
{"rae", XRP( 999) + reserve (env, 2) + 1*f,    1000,   noPreTrust,     1000,             tesSUCCESS, XRP(1000) + f, USD(   1000),      0,     1},
{"sue", XRP(1000) + reserve (env, 2) + 1*f,       0,   noPreTrust,     1000,             tesSUCCESS,             f, USD(      0),      1,     1},

{"abe",             reserve (env, 0) + 0*f,       1,   gwPreTrust,     1000,      tecUNFUNDED_OFFER,             f, USD(      0),      0,     0},
{"bud",             reserve (env, 0) + 1*f,       1,   gwPreTrust,     1000,      tecUNFUNDED_OFFER,             f, USD(      0),      0,     0},
{"che",             reserve (env, 0) + 2*f,       0,   gwPreTrust,     1000, tecINSUF_RESERVE_OFFER,             f, USD(      0),      0,     0},
{"dan",             reserve (env, 0) + 2*f,       1,   gwPreTrust,     1000,             tesSUCCESS,           2*f, USD(0.00001),      0,     0},
{"eli", XRP(  20) + reserve (env, 0) + 1*f,    1000,   gwPreTrust,     1000,             tesSUCCESS, XRP(20) + 1*f, USD(     20),      0,     0},
{"fyn",             reserve (env, 1) + 0*f,       0,   gwPreTrust,     1000,             tesSUCCESS,             f, USD(      0),      1,     1},
{"gar",             reserve (env, 1) + 0*f,       1,   gwPreTrust,     1000,             tesSUCCESS, XRP(   1) + f, USD(      1),      1,     1},
{"hal",             reserve (env, 1) + 1*f,       1,   gwPreTrust,     1000,             tesSUCCESS, XRP(   1) + f, USD(      1),      1,     1},

{"ned",             reserve (env, 1) + 0*f,       1, acctPreTrust,     1000,      tecUNFUNDED_OFFER,           2*f, USD(      0),      0,     1},
{"ole",             reserve (env, 1) + 1*f,       1, acctPreTrust,     1000,      tecUNFUNDED_OFFER,           2*f, USD(      0),      0,     1},
{"pat",             reserve (env, 1) + 2*f,       0, acctPreTrust,     1000,      tecUNFUNDED_OFFER,           2*f, USD(      0),      0,     1},
{"quy",             reserve (env, 1) + 2*f,       1, acctPreTrust,     1000,      tecUNFUNDED_OFFER,           2*f, USD(      0),      0,     1},
{"ron",             reserve (env, 1) + 3*f,       0, acctPreTrust,     1000, tecINSUF_RESERVE_OFFER,           2*f, USD(      0),      0,     1},
{"syd",             reserve (env, 1) + 3*f,       1, acctPreTrust,     1000,             tesSUCCESS,           3*f, USD(0.00001),      0,     1},
{"ted", XRP(  20) + reserve (env, 1) + 2*f,    1000, acctPreTrust,     1000,             tesSUCCESS, XRP(20) + 2*f, USD(     20),      0,     1},
{"uli",             reserve (env, 2) + 0*f,       0, acctPreTrust,     1000, tecINSUF_RESERVE_OFFER,           2*f, USD(      0),      0,     1},
{"vic",             reserve (env, 2) + 0*f,       1, acctPreTrust,     1000,             tesSUCCESS, XRP( 1) + 2*f, USD(      1),      0,     1},
{"wes",             reserve (env, 2) + 1*f,       0, acctPreTrust,     1000,             tesSUCCESS,           2*f, USD(      0),      1,     2},
{"xan",             reserve (env, 2) + 1*f,       1, acctPreTrust,     1000,             tesSUCCESS, XRP( 1) + 2*f, USD(      1),      1,     2},
};

        for (auto const& t : tests)
        {
            auto const acct = Account(t.account);
            env.fund (t.fundXrp, acct);
            env.close();

            env.require (offers (gw, 0));

            auto const book = t.bookAmount;
            if (book)
                env (offer (gw, XRP (book), USD (book)));
            env.close();
            std::uint32_t const gwOfferSeq = env.seq (gw) - 1;

            if (t.preTrust == gwPreTrust)
                env (trust (gw, acct["USD"] (1)));

            if (t.preTrust == acctPreTrust)
                env (trust (acct, USD (1)));

            env.close();

            auto const acctOffer = t.offerAmount;
            env (offer (acct, USD (acctOffer), XRP (acctOffer)), ter (t.tec));
            env.close();
            std::uint32_t const acctOfferSeq = env.seq (acct) - 1;

            BEAST_EXPECT (env.balance (acct, USD.issue()) == t.balanceUsd);
            BEAST_EXPECT (
                env.balance (acct, xrpIssue()) == t.fundXrp - t.spentXrp);
            env.require (offers (acct, t.offers));
            env.require (owners (acct, t.owners));

            auto acctOffers = offersOnAccount (env, acct);
            BEAST_EXPECT (acctOffers.size() == t.offers);
            if (acctOffers.size() && t.offers)
            {
                auto const& acctOffer = *(acctOffers.front());

                auto const leftover = t.offerAmount - t.bookAmount;
                BEAST_EXPECT (acctOffer[sfTakerGets] == XRP (leftover));
                BEAST_EXPECT (acctOffer[sfTakerPays] == USD (leftover));
            }

            if (t.preTrust == noPreTrust)
            {
                if (t.balanceUsd.value().signum())
                {
                    verifyDefaultTrustline (env, acct, t.balanceUsd);
                }
                else
                {
                    auto const sleTrust =
                        env.le (keylet::line(acct, USD.issue()));
                    BEAST_EXPECT (! sleTrust);
                }
            }

            env (offer_cancel (acct, acctOfferSeq));
            env (offer_cancel (gw, gwOfferSeq));
            env.close();
        }
    }

    void
    testXRPDirectCross (FeatureBitset features)
    {
        testcase ("XRP Direct Crossing");

        using namespace jtx;

        auto const gw = Account("gateway");
        auto const alice = Account("alice");
        auto const bob = Account("bob");
        auto const USD = gw["USD"];

        auto const usdOffer = USD(1000);
        auto const xrpOffer = XRP(1000);

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        env.fund (XRP(1000000), gw, bob);
        env.close();

        auto const fee = env.current ()->fees ().base;

        env.fund (reserve (env, 2) + (fee * 2), alice);
        env.close();

        env (trust(alice, usdOffer));

        env.close();

        env (pay(gw, alice, usdOffer));
        env.close();
        env.require (
            balance (alice, usdOffer),
            offers (alice, 0),
            offers (bob, 0));

        auto const alicesXRP = env.balance (alice);
        auto const bobsXRP = env.balance (bob);

        env (offer (alice, xrpOffer, usdOffer));
        env.close();
        env (offer (bob, usdOffer, xrpOffer));

        env.close();
        env.require (
            balance (alice, USD(0)),
            balance (bob, usdOffer),
            balance (alice, alicesXRP + xrpOffer - fee),
            balance (bob,   bobsXRP   - xrpOffer - fee),
            offers (alice, 0),
            offers (bob, 0));

        verifyDefaultTrustline (env, bob, usdOffer);

        env (offer (alice, USD(999), XRP(999)));
        env (offer (bob, xrpOffer, usdOffer));

        env.close();
        env.require (balance (alice, USD(999)));
        env.require (balance (bob, USD(1)));
        env.require (offers (alice, 0));
        verifyDefaultTrustline (env, bob, USD(1));
        {
            auto const bobsOffers = offersOnAccount (env, bob);
            BEAST_EXPECT (bobsOffers.size() == 1);
            auto const& bobsOffer = *(bobsOffers.front());

            BEAST_EXPECT (bobsOffer[sfLedgerEntryType] == ltOFFER);
            BEAST_EXPECT (bobsOffer[sfTakerGets] == USD (1));
            BEAST_EXPECT (bobsOffer[sfTakerPays] == XRP (1));
        }
   }

    void
    testDirectCross (FeatureBitset features)
    {
        testcase ("Direct Crossing");

        using namespace jtx;

        auto const gw = Account("gateway");
        auto const alice = Account("alice");
        auto const bob = Account("bob");
        auto const USD = gw["USD"];
        auto const EUR = gw["EUR"];

        auto const usdOffer = USD(1000);
        auto const eurOffer = EUR(1000);

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        env.fund (XRP(1000000), gw);
        env.close();

        auto const fee = env.current ()->fees ().base;

        env.fund (reserve (env, 3) + (fee * 3), alice);
        env.fund (reserve (env, 3) + (fee * 2), bob);
        env.close();
        env (trust(alice, usdOffer));
        env (trust(bob, eurOffer));
        env.close();

        env (pay(gw, alice, usdOffer));
        env (pay(gw, bob, eurOffer));
        env.close();
        env.require (
            balance (alice, usdOffer),
            balance (bob, eurOffer));

        env (offer (alice, eurOffer, usdOffer));
        env (offer (bob, usdOffer, eurOffer));

        env.close();
        env.require (
            balance (alice, eurOffer),
            balance (bob, usdOffer),
            offers (alice, 0),
            offers (bob, 0));
        verifyDefaultTrustline (env, alice, eurOffer);
        verifyDefaultTrustline (env, bob, usdOffer);

        env (offer (alice, USD(999), eurOffer));
        env (offer (bob, eurOffer, usdOffer));

        env.close();
        env.require (balance (alice, USD(999)));
        env.require (balance (alice, EUR(1)));
        env.require (balance (bob, USD(1)));
        env.require (balance (bob, EUR(999)));
        env.require (offers (alice, 0));
        verifyDefaultTrustline (env, alice, EUR(1));
        verifyDefaultTrustline (env, bob, USD(1));
        {
            auto bobsOffers = offersOnAccount (env, bob);
            BEAST_EXPECT (bobsOffers.size() == 1);
            auto const& bobsOffer = *(bobsOffers.front());

            BEAST_EXPECT (bobsOffer[sfTakerGets] == USD (1));
            BEAST_EXPECT (bobsOffer[sfTakerPays] == EUR (1));
        }

        env (offer (alice, USD(1), EUR(1)));

        env.close();
        env.require (balance (alice, USD(1000)));
        env.require (balance (alice, EUR(none)));
        env.require (balance (bob, USD(none)));
        env.require (balance (bob, EUR(1000)));
        env.require (offers (alice, 0));
        env.require (offers (bob, 0));

        BEAST_EXPECT (! env.le (keylet::line (alice.id(), EUR.issue())));
        BEAST_EXPECT (! env.le (keylet::line (bob.id(), USD.issue())));
    }

    void
    testBridgedCross (FeatureBitset features)
    {
        testcase ("Bridged Crossing");

        using namespace jtx;

        auto const gw    = Account("gateway");
        auto const alice = Account("alice");
        auto const bob   = Account("bob");
        auto const carol = Account("carol");
        auto const USD = gw["USD"];
        auto const EUR = gw["EUR"];

        auto const usdOffer = USD(1000);
        auto const eurOffer = EUR(1000);

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        env.fund (XRP(1000000), gw, alice, bob, carol);
        env.close();

        env (trust(alice, usdOffer));
        env (trust(carol, eurOffer));
        env.close();
        env (pay(gw, alice, usdOffer));
        env (pay(gw, carol, eurOffer));
        env.close();

        env (offer (alice, XRP(1000), usdOffer));
        env (offer (bob, eurOffer, XRP(1000)));
        auto const bobXrpBalance = env.balance (bob);
        env.close();

        env (offer (carol, USD(400), EUR(400)));
        env.close();

        env.require (
            balance (alice, USD(600)),
            balance (bob,   EUR(400)),
            balance (carol, USD(400)),
            balance (bob, bobXrpBalance - XRP(400)),
            offers (carol, 0));
        verifyDefaultTrustline (env, bob, EUR(400));
        verifyDefaultTrustline (env, carol, USD(400));
        {
            auto const alicesOffers = offersOnAccount (env, alice);
            BEAST_EXPECT (alicesOffers.size() == 1);
            auto const& alicesOffer = *(alicesOffers.front());

            BEAST_EXPECT (alicesOffer[sfLedgerEntryType] == ltOFFER);
            BEAST_EXPECT (alicesOffer[sfTakerGets] == USD (600));
            BEAST_EXPECT (alicesOffer[sfTakerPays] == XRP (600));
        }
        {
            auto const bobsOffers = offersOnAccount (env, bob);
            BEAST_EXPECT (bobsOffers.size() == 1);
            auto const& bobsOffer = *(bobsOffers.front());

            BEAST_EXPECT (bobsOffer[sfLedgerEntryType] == ltOFFER);
            BEAST_EXPECT (bobsOffer[sfTakerGets] == XRP (600));
            BEAST_EXPECT (bobsOffer[sfTakerPays] == EUR (600));
        }

        env (offer (carol, USD(600), EUR(600)));
        env.close();

        env.require (
            balance (alice, USD(0)),
            balance (bob, eurOffer),
            balance (carol, usdOffer),
            balance (bob, bobXrpBalance - XRP(1000)),
            offers (bob, 0),
            offers (carol, 0));
        verifyDefaultTrustline (env, bob, EUR(1000));
        verifyDefaultTrustline (env, carol, USD(1000));

        auto const alicesOffers = offersOnAccount (env, alice);
        if (alicesOffers.size() != 0)
        {
            BEAST_EXPECT (alicesOffers.size() == 1);
            auto const& alicesOffer = *(alicesOffers.front());

            BEAST_EXPECT (alicesOffer[sfLedgerEntryType] == ltOFFER);
            BEAST_EXPECT (alicesOffer[sfTakerGets] == USD (0));
            BEAST_EXPECT (alicesOffer[sfTakerPays] == XRP (0));
        }
    }

    void
    testSellOffer (FeatureBitset features)
    {
        testcase ("Sell Offer");

        using namespace jtx;

        auto const gw = Account("gateway");
        auto const USD = gw["USD"];

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        env.fund (XRP(10000000), gw);

        auto const f = env.current ()->fees ().base;

        enum preTrustType {noPreTrust, gwPreTrust, acctPreTrust};
        struct TestData
        {
            std::string account;       
            STAmount fundXrp;          
            STAmount fundUSD;          
            STAmount gwGets;           
            STAmount gwPays;           
            STAmount acctGets;         
            STAmount acctPays;         
            TER tec;                   
            STAmount spentXrp;         
            STAmount finalUsd;         
            int offers;                
            int owners;                
            STAmount takerGets;        
            STAmount takerPays;        

            TestData (
                std::string&& account_,         
                STAmount const& fundXrp_,       
                STAmount const& fundUSD_,       
                STAmount const& gwGets_,        
                STAmount const& gwPays_,        
                STAmount const& acctGets_,      
                STAmount const& acctPays_,      
                TER tec_,                       
                STAmount const& spentXrp_,      
                STAmount const& finalUsd_,      
                int offers_,                    
                int owners_,                    
                STAmount const& takerGets_,     
                STAmount const& takerPays_)     
                : account (std::move(account_))
                , fundXrp (fundXrp_)
                , fundUSD (fundUSD_)
                , gwGets (gwGets_)
                , gwPays (gwPays_)
                , acctGets (acctGets_)
                , acctPays (acctPays_)
                , tec (tec_)
                , spentXrp (spentXrp_)
                , finalUsd (finalUsd_)
                , offers (offers_)
                , owners (owners_)
                , takerGets (takerGets_)
                , takerPays (takerPays_)
            { }

            TestData (
                std::string&& account_,         
                STAmount const& fundXrp_,       
                STAmount const& fundUSD_,       
                STAmount const& gwGets_,        
                STAmount const& gwPays_,        
                STAmount const& acctGets_,      
                STAmount const& acctPays_,      
                TER tec_,                       
                STAmount const& spentXrp_,      
                STAmount const& finalUsd_,      
                int offers_,                    
                int owners_)                    
                : TestData (std::move(account_), fundXrp_, fundUSD_, gwGets_,
                  gwPays_, acctGets_, acctPays_, tec_, spentXrp_, finalUsd_,
                  offers_, owners_, STAmount {0}, STAmount {0})
            { }
        };

        TestData const tests[]
        {
{"ann", XRP(10) + reserve (env, 0) + 1*f, USD( 0), XRP(10), USD( 5),  USD(10),  XRP(10), tecINSUF_RESERVE_OFFER, XRP( 0) + (1*f),  USD( 0),      0,     0},
{"bev", XRP(10) + reserve (env, 1) + 1*f, USD( 0), XRP(10), USD( 5),  USD(10),  XRP(10),             tesSUCCESS, XRP( 0) + (1*f),  USD( 0),      1,     1,     XRP(10),  USD(10)},
{"cam", XRP(10) + reserve (env, 0) + 1*f, USD( 0), XRP(10), USD(10),  USD(10),  XRP(10),             tesSUCCESS, XRP(10) + (1*f),  USD(10),      0,     1},
{"deb", XRP(10) + reserve (env, 0) + 1*f, USD( 0), XRP(10), USD(20),  USD(10),  XRP(10),             tesSUCCESS, XRP(10) + (1*f),  USD(20),      0,     1},
{"eve", XRP(10) + reserve (env, 0) + 1*f, USD( 0), XRP(10), USD(20),  USD( 5),  XRP( 5),             tesSUCCESS, XRP( 5) + (1*f),  USD(10),      0,     1},
{"flo", XRP(10) + reserve (env, 0) + 1*f, USD( 0), XRP(10), USD(20),  USD(20),  XRP(20),             tesSUCCESS, XRP(10) + (1*f),  USD(20),      0,     1},
{"gay", XRP(20) + reserve (env, 1) + 1*f, USD( 0), XRP(10), USD(20),  USD(20),  XRP(20),             tesSUCCESS, XRP(10) + (1*f),  USD(20),      0,     1},
{"hye", XRP(20) + reserve (env, 2) + 1*f, USD( 0), XRP(10), USD(20),  USD(20),  XRP(20),             tesSUCCESS, XRP(10) + (1*f),  USD(20),      1,     2,     XRP(10),  USD(10)},
{"meg",           reserve (env, 1) + 2*f, USD(10), USD(10), XRP( 5),  XRP(10),  USD(10), tecINSUF_RESERVE_OFFER, XRP(  0) + (2*f),  USD(10),      0,     1},
{"nia",           reserve (env, 2) + 2*f, USD(10), USD(10), XRP( 5),  XRP(10),  USD(10),             tesSUCCESS, XRP(  0) + (2*f),  USD(10),      1,     2,     USD(10),  XRP(10)},
{"ova",           reserve (env, 1) + 2*f, USD(10), USD(10), XRP(10),  XRP(10),  USD(10),             tesSUCCESS, XRP(-10) + (2*f),  USD( 0),      0,     1},
{"pam",           reserve (env, 1) + 2*f, USD(10), USD(10), XRP(20),  XRP(10),  USD(10),             tesSUCCESS, XRP(-20) + (2*f),  USD( 0),      0,     1},
{"qui",           reserve (env, 1) + 2*f, USD(10), USD(20), XRP(40),  XRP(10),  USD(10),             tesSUCCESS, XRP(-20) + (2*f),  USD( 0),      0,     1},
{"rae",           reserve (env, 2) + 2*f, USD(10), USD( 5), XRP( 5),  XRP(10),  USD(10),             tesSUCCESS, XRP( -5) + (2*f),  USD( 5),      1,     2,     USD( 5),  XRP( 5)},
{"sue",           reserve (env, 2) + 2*f, USD(10), USD( 5), XRP(10),  XRP(10),  USD(10),             tesSUCCESS, XRP(-10) + (2*f),  USD( 5),      1,     2,     USD( 5),  XRP( 5)},
};
        auto const zeroUsd = USD(0);
        for (auto const& t : tests)
        {
            env.require (offers (gw, 0));

            auto const acct = Account(t.account);

            env.fund (t.fundXrp, acct);
            env.close();

            if (t.fundUSD != zeroUsd)
            {
                env (trust (acct, t.fundUSD));
                env.close();
                env (pay (gw, acct, t.fundUSD));
                env.close();
            }

            env (offer (gw, t.gwGets, t.gwPays));
            env.close();
            std::uint32_t const gwOfferSeq = env.seq (gw) - 1;

            env (offer (acct, t.acctGets, t.acctPays, tfSell), ter (t.tec));
            env.close();
            std::uint32_t const acctOfferSeq = env.seq (acct) - 1;

            BEAST_EXPECT (env.balance (acct, USD.issue()) == t.finalUsd);
            BEAST_EXPECT (
                env.balance (acct, xrpIssue()) == t.fundXrp - t.spentXrp);
            env.require (offers (acct, t.offers));
            env.require (owners (acct, t.owners));

            if (t.offers)
            {
                auto const acctOffers = offersOnAccount (env, acct);
                if (acctOffers.size() > 0)
                {
                    BEAST_EXPECT (acctOffers.size() == 1);
                    auto const& acctOffer = *(acctOffers.front());

                    BEAST_EXPECT (acctOffer[sfLedgerEntryType] == ltOFFER);
                    BEAST_EXPECT (acctOffer[sfTakerGets] == t.takerGets);
                    BEAST_EXPECT (acctOffer[sfTakerPays] == t.takerPays);
                }
            }

            env (offer_cancel (acct, acctOfferSeq));
            env (offer_cancel (gw, gwOfferSeq));
            env.close();
        }
    }

    void
    testSellWithFillOrKill (FeatureBitset features)
    {
        testcase ("Combine tfSell with tfFillOrKill");

        using namespace jtx;

        auto const gw    = Account("gateway");
        auto const alice = Account("alice");
        auto const bob   = Account("bob");
        auto const USD   = gw["USD"];

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        env.fund (XRP(10000000), gw, alice, bob);

        TER const killedCode {
            features[fix1578] ? TER {tecKILLED} : TER {tesSUCCESS}};

        env (trust(bob, USD(200)));
        env.close();
        env (pay(gw, bob, USD(100)));
        env.close();
        env (offer(bob, XRP(2000), USD(20)));
        env.close();
        {
            env (offer(alice, USD(21), XRP(2100),
                tfSell | tfFillOrKill), ter (killedCode));
            env.close();
            env.require (balance (alice, USD(none)));
            env.require (offers (alice, 0));
            env.require (balance (bob, USD(100)));
        }
        {
            env (offer(alice, USD(20), XRP(2000), tfSell | tfFillOrKill));
            env.close();
            env.require (balance (alice, USD(20)));
            env.require (offers (alice, 0));
            env.require (balance (bob, USD(80)));
        }
        {
            env (offer(bob, XRP(2000), USD(20)));
            env.close();
            env (offer(alice, USD(10), XRP(1500), tfSell | tfFillOrKill));
            env.close();
            env.require (balance (alice, USD(35)));
            env.require (offers (alice, 0));
            env.require (balance (bob, USD(65)));
        }
        {

            env (offer(alice, USD(1), XRP(501),
                tfSell | tfFillOrKill), ter (killedCode));
            env.close();
            env.require (balance (alice, USD(35)));
            env.require (offers (alice, 0));
            env.require (balance (bob, USD(65)));
        }
        {

            env (offer(alice, USD(1), XRP(500), tfSell | tfFillOrKill));
            env.close();
            env.require (balance (alice, USD(40)));
            env.require (offers (alice, 0));
            env.require (balance (bob, USD(60)));
        }
    }

    void
    testTransferRateOffer (FeatureBitset features)
    {
        testcase ("Transfer Rate Offer");

        using namespace jtx;

        auto const gw1 = Account("gateway1");
        auto const USD = gw1["USD"];

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        auto const fee = env.current ()->fees ().base;

        env.fund (XRP(100000), gw1);
        env.close();

        env(rate(gw1, 1.25));
        {
            auto const ann = Account("ann");
            auto const bob = Account("bob");
            env.fund (XRP(100) + reserve(env, 2) + (fee*2), ann, bob);
            env.close();

            env (trust(ann, USD(200)));
            env (trust(bob, USD(200)));
            env.close();

            env (pay (gw1, bob, USD(125)));
            env.close();

            env (offer (bob, XRP(1), USD(100)));
            env.close();

            env (offer (ann, USD(100), XRP(1)));
            env.close();

            env.require (balance (ann, USD(100)));
            env.require (balance (ann, XRP( 99) + reserve(env, 2)));
            env.require (offers (ann, 0));

            env.require (balance (bob, USD(  0)));
            env.require (balance (bob, XRP(101) + reserve(env, 2)));
            env.require (offers (bob, 0));
        }
        {
            auto const che = Account("che");
            auto const deb = Account("deb");
            env.fund (XRP(100) + reserve(env, 2) + (fee*2), che, deb);
            env.close();

            env (trust(che, USD(200)));
            env (trust(deb, USD(200)));
            env.close();

            env (pay (gw1, deb, USD(125)));
            env.close();

            env (offer (che, USD(100), XRP(1)));
            env.close();

            env (offer (deb, XRP(1), USD(100)));
            env.close();

            env.require (balance (che, USD(100)));
            env.require (balance (che, XRP( 99) + reserve(env, 2)));
            env.require (offers (che, 0));

            env.require (balance (deb, USD(  0)));
            env.require (balance (deb, XRP(101) + reserve(env, 2)));
            env.require (offers (deb, 0));
        }
        {
            auto const eve = Account("eve");
            auto const fyn = Account("fyn");

            env.fund (XRP(20000) + fee*2, eve, fyn);
            env.close();

            env (trust (eve, USD(1000)));
            env (trust (fyn, USD(1000)));
            env.close();

            env (pay (gw1, eve, USD(100)));
            env (pay (gw1, fyn, USD(100)));
            env.close();

            env (offer (eve, USD(10), XRP(4000)));
            env.close();
            std::uint32_t const eveOfferSeq = env.seq (eve) - 1;

            env (offer (fyn, XRP(2000), USD(5)));
            env.close();

            env.require (balance (eve, USD(105)));
            env.require (balance (eve, XRP(18000)));
            auto const evesOffers = offersOnAccount (env, eve);
            BEAST_EXPECT (evesOffers.size() == 1);
            if (evesOffers.size() != 0)
            {
                auto const& evesOffer = *(evesOffers.front());
                BEAST_EXPECT (evesOffer[sfLedgerEntryType] == ltOFFER);
                BEAST_EXPECT (evesOffer[sfTakerGets] == XRP (2000));
                BEAST_EXPECT (evesOffer[sfTakerPays] == USD (5));
            }
            env (offer_cancel (eve, eveOfferSeq)); 

            env.require (balance (fyn, USD(93.75)));
            env.require (balance (fyn, XRP(22000)));
            env.require (offers (fyn, 0));
        }
        auto const gw2   = Account("gateway2");
        auto const EUR = gw2["EUR"];

        env.fund (XRP(100000), gw2);
        env.close();

        env(rate(gw2, 1.5));
        {
            auto const gay = Account("gay");
            auto const hal = Account("hal");
            env.fund (reserve(env, 3) + (fee*3), gay, hal);
            env.close();

            env (trust(gay, USD(200)));
            env (trust(gay, EUR(200)));
            env (trust(hal, USD(200)));
            env (trust(hal, EUR(200)));
            env.close();

            env (pay (gw1, gay, USD(125)));
            env (pay (gw2, hal, EUR(150)));
            env.close();

            env (offer (gay, EUR(100), USD(100)));
            env.close();

            env (offer (hal, USD(100), EUR(100)));
            env.close();

            env.require (balance (gay, USD(  0)));
            env.require (balance (gay, EUR(100)));
            env.require (balance (gay, reserve(env, 3)));
            env.require (offers (gay, 0));

            env.require (balance (hal, USD(100)));
            env.require (balance (hal, EUR(  0)));
            env.require (balance (hal, reserve(env, 3)));
            env.require (offers (hal, 0));
        }
        {
            auto const ivy = Account("ivy");
            auto const joe = Account("joe");
            env.fund (reserve(env, 3) + (fee*3), ivy, joe);
            env.close();

            env (trust(ivy, USD(400)), qualityInPercent (90));
            env (trust(ivy, EUR(400)), qualityInPercent (80));
            env (trust(joe, USD(400)), qualityInPercent (70));
            env (trust(joe, EUR(400)), qualityInPercent (60));
            env.close();

            env (pay (gw1, ivy, USD(270)), sendmax (USD(500)));
            env (pay (gw2, joe, EUR(150)), sendmax (EUR(300)));
            env.close();
            env.require (balance (ivy, USD(300)));
            env.require (balance (joe, EUR(250)));

            env (offer (ivy, EUR(100), USD(200)));
            env.close();

            env (offer (joe, USD(200), EUR(100)));
            env.close();

            env.require (balance (ivy, USD( 50)));
            env.require (balance (ivy, EUR(100)));
            env.require (balance (ivy, reserve(env, 3)));
            env.require (offers (ivy, 0));

            env.require (balance (joe, USD(200)));
            env.require (balance (joe, EUR(100)));
            env.require (balance (joe, reserve(env, 3)));
            env.require (offers (joe, 0));
        }
        {
            auto const kim = Account("kim");
            auto const K_BUX = kim["BUX"];
            auto const lex = Account("lex");
            auto const meg = Account("meg");
            auto const ned = Account("ned");
            auto const N_BUX = ned["BUX"];

            env.fund (reserve(env, 4) + (fee*4), kim, lex, meg, ned);
            env.close();

            env (trust (lex, K_BUX(400)));
            env (trust (lex, N_BUX(200)), qualityOutPercent (120));
            env (trust (meg, N_BUX(100)));
            env.close();
            env (pay (ned, lex, N_BUX(100)));
            env.close();
            env.require (balance (lex, N_BUX(100)));

            env (pay (kim, meg,
                N_BUX(60)), path (lex, ned), sendmax (K_BUX(200)));
            env.close();

            env.require (balance (kim, K_BUX(none)));
            env.require (balance (kim, N_BUX(none)));
            env.require (balance (lex, K_BUX(  72)));
            env.require (balance (lex, N_BUX(  40)));
            env.require (balance (meg, K_BUX(none)));
            env.require (balance (meg, N_BUX(  60)));
            env.require (balance (ned, K_BUX(none)));
            env.require (balance (ned, N_BUX(none)));

            env (offer (lex, K_BUX(30), N_BUX(30)));
            env.close();

            env (offer (kim, N_BUX(30), K_BUX(30)));
            env.close();

            env.require (balance (kim, K_BUX(none)));
            env.require (balance (kim, N_BUX(  30)));
            env.require (balance (lex, K_BUX( 102)));
            env.require (balance (lex, N_BUX(  10)));
            env.require (balance (meg, K_BUX(none)));
            env.require (balance (meg, N_BUX(  60)));
            env.require (balance (ned, K_BUX( -30)));
            env.require (balance (ned, N_BUX(none)));
        }
        {
            auto const ova = Account("ova");
            auto const pat = Account("pat");
            auto const qae = Account("qae");
            env.fund (XRP(2) + reserve(env, 3) + (fee*3), ova, pat, qae);
            env.close();

            env (trust(ova, USD(200)));
            env (trust(ova, EUR(200)));
            env (trust(pat, USD(200)));
            env (trust(pat, EUR(200)));
            env (trust(qae, USD(200)));
            env (trust(qae, EUR(200)));
            env.close();

            env (pay (gw1, ova, USD(125)));
            env (pay (gw2, qae, EUR(150)));
            env.close();

            env (offer (ova, XRP(2), USD(100)));
            env (offer (pat, EUR(100), XRP(2)));
            env.close();

            env (offer (qae, USD(100), EUR(100)));
            env.close();

            env.require (balance (ova, USD(  0)));
            env.require (balance (ova, EUR(  0)));
            env.require (balance (ova, XRP(4) + reserve(env, 3)));

            auto const ovasOffers = offersOnAccount (env, ova);
            if (ovasOffers.size() != 0)
            {
                BEAST_EXPECT (ovasOffers.size() == 1);
                auto const& ovasOffer = *(ovasOffers.front());

                BEAST_EXPECT (ovasOffer[sfLedgerEntryType] == ltOFFER);
                BEAST_EXPECT (ovasOffer[sfTakerGets] == USD (0));
                BEAST_EXPECT (ovasOffer[sfTakerPays] == XRP (0));
            }

            env.require (balance (pat, USD(  0)));
            env.require (balance (pat, EUR(100)));
            env.require (balance (pat, XRP(0) + reserve(env, 3)));
            env.require (offers (pat, 0));

            env.require (balance (qae, USD(100)));
            env.require (balance (qae, EUR(  0)));
            env.require (balance (qae, XRP(2) + reserve(env, 3)));
            env.require (offers (qae, 0));
        }
    }

    void
    testSelfCrossOffer1 (FeatureBitset features)
    {
        using namespace jtx;

        auto const gw = Account("gateway");
        auto const USD = gw["USD"];

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        auto const fee = env.current ()->fees ().base;
        auto const startBalance = XRP(1000000);

        env.fund (startBalance + (fee*4), gw);
        env.close();

        env (offer (gw, USD(60), XRP(600)));
        env.close();
        env (offer (gw, USD(60), XRP(600)));
        env.close();
        env (offer (gw, USD(60), XRP(600)));
        env.close();

        env.require (owners (gw, 3));
        env.require (balance (gw, startBalance + fee));

        auto gwOffers = offersOnAccount (env, gw);
        BEAST_EXPECT (gwOffers.size() == 3);
        for (auto const& offerPtr : gwOffers)
        {
            auto const& offer = *offerPtr;
            BEAST_EXPECT (offer[sfLedgerEntryType] == ltOFFER);
            BEAST_EXPECT (offer[sfTakerGets] == XRP (600));
            BEAST_EXPECT (offer[sfTakerPays] == USD ( 60));
        }

        env (offer (gw, XRP(1000), USD(100)));
        env.close();
        env.require (owners (gw, 1));
        env.require (offers (gw, 1));
        env.require (balance (gw, startBalance));

        gwOffers = offersOnAccount (env, gw);
        BEAST_EXPECT (gwOffers.size() == 1);
        for (auto const& offerPtr : gwOffers)
        {
            auto const& offer = *offerPtr;
            BEAST_EXPECT (offer[sfLedgerEntryType] == ltOFFER);
            BEAST_EXPECT (offer[sfTakerGets] == USD (100));
            BEAST_EXPECT (offer[sfTakerPays] == XRP (1000));
        }
    }

    void
    testSelfCrossOffer2 (FeatureBitset features)
    {
        using namespace jtx;

        auto const gw1 = Account("gateway1");
        auto const gw2 = Account("gateway2");
        auto const alice = Account("alice");
        auto const USD = gw1["USD"];
        auto const EUR = gw2["EUR"];

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        env.fund (XRP(1000000), gw1, gw2);
        env.close();

        auto const f = env.current ()->fees ().base;

        struct TestData
        {
            std::string acct;          
            STAmount fundXRP;          
            STAmount fundUSD;          
            STAmount fundEUR;          
            TER firstOfferTec;         
            TER secondOfferTec;        
        };

        TestData const tests[]
        {
{"ann", reserve(env, 3) + f*4, USD(1000), EUR(1000),             tesSUCCESS,             tesSUCCESS},
{"bev", reserve(env, 3) + f*4, USD(   1), EUR(1000),             tesSUCCESS,             tesSUCCESS},
{"cam", reserve(env, 3) + f*4, USD(1000), EUR(   1),             tesSUCCESS,             tesSUCCESS},
{"deb", reserve(env, 3) + f*4, USD(   0), EUR(   1),             tesSUCCESS,      tecUNFUNDED_OFFER},
{"eve", reserve(env, 3) + f*4, USD(   1), EUR(   0),      tecUNFUNDED_OFFER,             tesSUCCESS},
{"flo", reserve(env, 3) +   0, USD(1000), EUR(1000), tecINSUF_RESERVE_OFFER, tecINSUF_RESERVE_OFFER},
        };

        for (auto const& t : tests)
        {
            auto const acct = Account {t.acct};
            env.fund (t.fundXRP, acct);
            env.close();

            env (trust (acct, USD(1000)));
            env (trust (acct, EUR(1000)));
            env.close();

            if (t.fundUSD > USD(0))
                env (pay (gw1, acct, t.fundUSD));
            if (t.fundEUR > EUR(0))
                env (pay (gw2, acct, t.fundEUR));
            env.close();

            env (offer (acct, USD(500), EUR(600)), ter (t.firstOfferTec));
            env.close();
            std::uint32_t const firstOfferSeq = env.seq (acct) - 1;

            int offerCount = t.firstOfferTec == tesSUCCESS ? 1 : 0;
            env.require (owners (acct, 2 + offerCount));
            env.require (balance (acct, t.fundUSD));
            env.require (balance (acct, t.fundEUR));

            auto acctOffers = offersOnAccount (env, acct);
            BEAST_EXPECT (acctOffers.size() == offerCount);
            for (auto const& offerPtr : acctOffers)
            {
                auto const& offer = *offerPtr;
                BEAST_EXPECT (offer[sfLedgerEntryType] == ltOFFER);
                BEAST_EXPECT (offer[sfTakerGets] == EUR (600));
                BEAST_EXPECT (offer[sfTakerPays] == USD (500));
            }

            env (offer (acct, EUR(600), USD(500)), ter (t.secondOfferTec));
            env.close();
            std::uint32_t const secondOfferSeq = env.seq (acct) - 1;

            offerCount = t.secondOfferTec == tesSUCCESS ? 1 : offerCount;
            env.require (owners (acct, 2 + offerCount));
            env.require (balance (acct, t.fundUSD));
            env.require (balance (acct, t.fundEUR));

            acctOffers = offersOnAccount (env, acct);
            BEAST_EXPECT (acctOffers.size() == offerCount);
            for (auto const& offerPtr : acctOffers)
            {
                auto const& offer = *offerPtr;
                BEAST_EXPECT (offer[sfLedgerEntryType] == ltOFFER);
                if (offer[sfSequence] == firstOfferSeq)
                {
                    BEAST_EXPECT (offer[sfTakerGets] == EUR (600));
                    BEAST_EXPECT (offer[sfTakerPays] == USD (500));
                }
                else
                {
                    BEAST_EXPECT (offer[sfTakerGets] == USD (500));
                    BEAST_EXPECT (offer[sfTakerPays] == EUR (600));
                }
            }

            env (offer_cancel (acct, firstOfferSeq));
            env.close();
            env (offer_cancel (acct, secondOfferSeq));
            env.close();
        }
    }

    void
    testSelfCrossOffer (FeatureBitset features)
    {
        testcase ("Self Cross Offer");
        testSelfCrossOffer1 (features);
        testSelfCrossOffer2 (features);
    }

    void
    testSelfIssueOffer (FeatureBitset features)
    {
        using namespace jtx;

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        auto const alice = Account("alice");
        auto const bob = Account("bob");
        auto const USD = bob["USD"];
        auto const f = env.current ()->fees ().base;

        env.fund(XRP(50000) + f, alice, bob);
        env.close();

        env(offer(alice, USD(5000), XRP(50000)));
        env.close();

        env(offer(bob, XRP(50000), USD(5000)));
        env.close();

        env.require (balance (alice, XRP(250)));
        env.require (owners (alice, 1));
        env.require (lines (alice, 1));

        auto const bobOffers = offersOnAccount (env, bob);
        BEAST_EXPECT(bobOffers.size() == 1);
        for (auto const& offerPtr : bobOffers)
        {
            auto const& offer = *offerPtr;
            BEAST_EXPECT(offer[sfLedgerEntryType] == ltOFFER);
            BEAST_EXPECT(offer[sfTakerGets] == USD ( 25));
            BEAST_EXPECT(offer[sfTakerPays] == XRP (250));
        }
    }

    void
    testBadPathAssert (FeatureBitset features)
    {
        testcase ("Bad path assert");

        using namespace jtx;

        Env env {*this, features | featureOwnerPaysFee};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        auto const fee = env.current ()->fees ().base;
        {
            auto const ann   = Account("ann");
            auto const A_BUX = ann["BUX"];
            auto const bob   = Account("bob");
            auto const cam   = Account("cam");
            auto const dan   = Account("dan");
            auto const D_BUX = dan["BUX"];

            env.fund (reserve(env, 4) + (fee*4), ann, bob, cam, dan);
            env.close();

            env (trust (bob, A_BUX(400)));
            env (trust (bob, D_BUX(200)), qualityOutPercent (120));
            env (trust (cam, D_BUX(100)));
            env.close();
            env (pay (dan, bob, D_BUX(100)));
            env.close();
            env.require (balance (bob, D_BUX(100)));

            env (pay (ann, cam, D_BUX(60)),
                path (bob, dan), sendmax (A_BUX(200)));
            env.close();

            env.require (balance (ann, A_BUX(none)));
            env.require (balance (ann, D_BUX(none)));
            env.require (balance (bob, A_BUX(  72)));
            env.require (balance (bob, D_BUX(  40)));
            env.require (balance (cam, A_BUX(none)));
            env.require (balance (cam, D_BUX(  60)));
            env.require (balance (dan, A_BUX(none)));
            env.require (balance (dan, D_BUX(none)));

            env (offer (bob, A_BUX(30), D_BUX(30)));
            env.close();

            env (trust (ann, D_BUX(100)));
            env.close();

            TER const tecExpect =
                features[featureFlow] ? TER {temBAD_PATH} : TER {tecPATH_DRY};

            env (pay (ann, ann, D_BUX(30)),
                path (A_BUX, D_BUX), sendmax (A_BUX(30)), ter (tecExpect));
            env.close();

            env.require (balance (ann, A_BUX(none)));
            env.require (balance (ann, D_BUX(   0)));
            env.require (balance (bob, A_BUX(  72)));
            env.require (balance (bob, D_BUX(  40)));
            env.require (balance (cam, A_BUX(none)));
            env.require (balance (cam, D_BUX(  60)));
            env.require (balance (dan, A_BUX(   0)));
            env.require (balance (dan, D_BUX(none)));
        }
    }

    void testDirectToDirectPath (FeatureBitset features)
    {
        testcase ("Direct to Direct path");

        using namespace jtx;

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        auto const ann = Account("ann");
        auto const bob = Account("bob");
        auto const cam = Account("cam");
        auto const A_BUX = ann["BUX"];
        auto const B_BUX = bob["BUX"];

        auto const fee = env.current ()->fees ().base;
        env.fund (reserve(env, 4) + (fee*5), ann, bob, cam);
        env.close();

        env (trust (ann, B_BUX(40)));
        env (trust (cam, A_BUX(40)));
        env (trust (cam, B_BUX(40)));
        env.close();

        env (pay (ann, cam, A_BUX(35)));
        env (pay (bob, cam, B_BUX(35)));

        env (offer (bob, A_BUX(30), B_BUX(30)));
        env.close();

        env (offer (cam, A_BUX(29), B_BUX(30), tfPassive));
        env.close();
        env.require (balance (cam, A_BUX(35)));
        env.require (balance (cam, B_BUX(35)));
        env.require (offers (cam, 1));

        env (offer (cam, B_BUX(30), A_BUX(30)));
        env.close();

        env.require (balance (bob, A_BUX(30)));
        env.require (balance (cam, A_BUX( 5)));
        env.require (balance (cam, B_BUX(65)));
        env.require (offers (cam, 0));
    }

    void testSelfCrossLowQualityOffer (FeatureBitset features)
    {
        testcase ("Self crossing low quality offer");

        using namespace jtx;

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        auto const ann = Account("ann");
        auto const gw = Account("gateway");
        auto const BTC = gw["BTC"];

        auto const fee = env.current ()->fees ().base;
        env.fund (reserve(env, 2) + drops (9999640) + (fee), ann);
        env.fund (reserve(env, 2) + (fee*4), gw);
        env.close();

        env (rate(gw, 1.002));
        env (trust(ann, BTC(10)));
        env.close();

        env (pay(gw, ann, BTC(2.856)));
        env.close();

        env (offer(ann, drops(365611702030), BTC(5.713)));
        env.close();

        env (offer(ann,
            BTC(0.687), drops(20000000000)), ter (tecINSUF_RESERVE_OFFER));
    }

    void testOfferInScaling (FeatureBitset features)
    {
        testcase ("Offer In Scaling");

        using namespace jtx;

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        auto const gw = Account("gateway");
        auto const alice = Account("alice");
        auto const bob = Account("bob");
        auto const CNY = gw["CNY"];

        auto const fee = env.current ()->fees ().base;
        env.fund (reserve(env, 2) + drops (400000000000) + (fee), alice, bob);
        env.fund (reserve(env, 2) + (fee*4), gw);
        env.close();

        env (trust(bob, CNY(500)));
        env.close();

        env (pay(gw, bob, CNY(300)));
        env.close();

        env (offer(bob, drops(5400000000), CNY(216.054)));
        env.close();

        env (offer(alice, CNY(13562.0001), drops(339000000000)));
        env.close();

        auto const aliceOffers = offersOnAccount (env, alice);
        BEAST_EXPECT(aliceOffers.size() == 1);
        for (auto const& offerPtr : aliceOffers)
        {
            auto const& offer = *offerPtr;
            BEAST_EXPECT(offer[sfLedgerEntryType] == ltOFFER);
            BEAST_EXPECT(offer[sfTakerGets] == drops (333599446582));
            BEAST_EXPECT(offer[sfTakerPays] == CNY (13345.9461));
        }
    }

    void testOfferInScalingWithXferRate (FeatureBitset features)
    {
        testcase ("Offer In Scaling With Xfer Rate");

        using namespace jtx;

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        auto const gw = Account("gateway");
        auto const alice = Account("alice");
        auto const bob = Account("bob");
        auto const BTC = gw["BTC"];
        auto const JPY = gw["JPY"];

        auto const fee = env.current ()->fees ().base;
        env.fund (reserve(env, 2) + drops (400000000000) + (fee), alice, bob);
        env.fund (reserve(env, 2) + (fee*4), gw);
        env.close();

        env (rate(gw, 1.002));
        env (trust(alice, JPY(4000)));
        env (trust(bob, BTC(2)));
        env.close();

        env (pay(gw, alice, JPY(3699.034802280317)));
        env (pay(gw, bob, BTC(1.156722559140311)));
        env.close();

        env (offer(bob, JPY(1241.913390770747), BTC(0.01969825690469254)));
        env.close();

        env (offer(alice, BTC(0.05507568706427876), JPY(3472.696773391072)));
        env.close();

        auto const aliceOffers = offersOnAccount (env, alice);
        BEAST_EXPECT(aliceOffers.size() == 1);
        for (auto const& offerPtr : aliceOffers)
        {
            auto const& offer = *offerPtr;
            BEAST_EXPECT(offer[sfLedgerEntryType] == ltOFFER);
            BEAST_EXPECT(offer[sfTakerGets] ==
                STAmount (JPY.issue(), std::uint64_t(2230682446713524ul), -12));
            BEAST_EXPECT(offer[sfTakerPays] == BTC (0.035378));
        }
    }

    void testOfferThresholdWithReducedFunds (FeatureBitset features)
    {
        testcase ("Offer Threshold With Reduced Funds");

        using namespace jtx;

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        auto const gw1 = Account("gw1");
        auto const gw2 = Account("gw2");
        auto const alice = Account("alice");
        auto const bob = Account("bob");
        auto const USD = gw1["USD"];
        auto const JPY = gw2["JPY"];

        auto const fee = env.current ()->fees ().base;
        env.fund (reserve(env, 2) + drops (400000000000) + (fee), alice, bob);
        env.fund (reserve(env, 2) + (fee*4), gw1, gw2);
        env.close();

        env (rate(gw1, 1.002));
        env (trust(alice, USD(1000)));
        env (trust(bob, JPY(100000)));
        env.close();

        env (pay(gw1, alice,
            STAmount {USD.issue(), std::uint64_t(2185410179555600), -14}));
        env (pay(gw2, bob,
            STAmount {JPY.issue(), std::uint64_t(6351823459548956), -12}));
        env.close();

        env (offer(bob,
            STAmount {USD.issue(), std::uint64_t(4371257532306000), -17},
            STAmount {JPY.issue(), std::uint64_t(4573216636606000), -15}));
        env.close();

        env (offer(alice,
            STAmount {JPY.issue(), std::uint64_t(2291181510070762), -12},
            STAmount {USD.issue(), std::uint64_t(2190218999914694), -14}));
        env.close();

        auto const aliceOffers = offersOnAccount (env, alice);
        BEAST_EXPECT(aliceOffers.size() == 1);
        for (auto const& offerPtr : aliceOffers)
        {
            auto const& offer = *offerPtr;
            BEAST_EXPECT(offer[sfLedgerEntryType] == ltOFFER);
            BEAST_EXPECT(offer[sfTakerGets] ==
                STAmount (USD.issue(), std::uint64_t(2185847305256635), -14));
            BEAST_EXPECT(offer[sfTakerPays] ==
                STAmount (JPY.issue(), std::uint64_t(2286608293434156), -12));
        }
    }

    void testTinyOffer (FeatureBitset features)
    {
        testcase ("Tiny Offer");

        using namespace jtx;

        Env env {*this, features};
        auto const closeTime =
            fix1449Time() +
                100 * env.closed()->info().closeTimeResolution;
        env.close (closeTime);

        auto const gw = Account("gw");
        auto const alice = Account("alice");
        auto const bob = Account("bob");
        auto const CNY = gw["CNY"];
        auto const fee = env.current()->fees().base;
        auto const startXrpBalance = drops (400000000000) + (fee * 2);

        env.fund (startXrpBalance, gw, alice, bob);
        env.close();

        env (trust (bob, CNY(100000)));
        env.close();

        STAmount const alicesCnyOffer {
            CNY.issue(), std::uint64_t(4926000000000000), -23 };

        env (offer (alice, alicesCnyOffer, drops (1), tfPassive));
        env.close();

        STAmount const bobsCnyStartBalance {
            CNY.issue(), std::uint64_t(3767479960090235), -15};
        env (pay(gw, bob, bobsCnyStartBalance));
        env.close();

        env (offer (bob, drops (203),
            STAmount {CNY.issue(), std::uint64_t(1000000000000000), -20}));
        env.close();

        env.require (balance (alice, alicesCnyOffer));
        env.require (balance (alice, startXrpBalance - fee - drops(1)));
        env.require (balance (bob, bobsCnyStartBalance - alicesCnyOffer));
        env.require (balance (bob, startXrpBalance - (fee * 2) + drops(1)));
    }

    void testSelfPayXferFeeOffer (FeatureBitset features)
    {
        testcase ("Self Pay Xfer Fee");

        using namespace jtx;

        Env env {*this, features};
        auto const closeTime =
            fix1449Time() +
                100 * env.closed()->info().closeTimeResolution;
        env.close (closeTime);

        auto const gw = Account("gw");
        auto const BTC = gw["BTC"];
        auto const USD = gw["USD"];
        auto const startXrpBalance = XRP (4000000);

        env.fund (startXrpBalance, gw);
        env.close();

        env (rate (gw, 1.25));
        env.close();

        struct Actor
        {
            Account acct;
            int offers;        
            PrettyAmount xrp;  
            PrettyAmount btc;  
            PrettyAmount usd;  
        };
        struct TestData
        {
            std::size_t self;
            std::size_t leg0;
            std::size_t leg1;
            PrettyAmount btcStart;
            std::vector<Actor> actors;
        };

        TestData const tests[]
        {
{ 0, 0, 1, BTC(20), { {"ann", 0, drops(3899999999960), BTC(20.0), USD(3000)}, {"abe", 0, drops(4099999999970), BTC( 0), USD(750)} } }, 
{ 0, 1, 0, BTC(20), { {"bev", 0, drops(4099999999960), BTC( 7.5), USD(2000)}, {"bob", 0, drops(3899999999970), BTC(10), USD(  0)} } }, 
{ 0, 0, 0, BTC(20), { {"cam", 0, drops(3999999999950), BTC(20.0), USD(2000)}                                                      } }, 
{ 0, 1, 0, BTC( 5), { {"eve", 1, drops(4039999999960), BTC( 0.0), USD(2000)}, {"eli", 1, drops(3959999999970), BTC( 4), USD(  0)} } }, 
        };

        for (auto const& t : tests)
        {
            Account const& self = t.actors[t.self].acct;
            Account const& leg0 = t.actors[t.leg0].acct;
            Account const& leg1 = t.actors[t.leg1].acct;

            for (auto const& actor : t.actors)
            {
                env.fund (XRP (4000000), actor.acct);
                env.close();

                env (trust (actor.acct, BTC(40)));
                env (trust (actor.acct, USD(8000)));
                env.close();
            }

            env (pay (gw, self, t.btcStart));
            env (pay (gw, self, USD(2000)));
            if (self.id() != leg1.id())
                env (pay (gw, leg1, USD(2000)));
            env.close();

            env (offer (leg0, BTC(10),     XRP(100000), tfPassive));
            env.close();
            std::uint32_t const leg0OfferSeq = env.seq (leg0) - 1;

            env (offer (leg1, XRP(100000), USD(1000),   tfPassive));
            env.close();
            std::uint32_t const leg1OfferSeq = env.seq (leg1) - 1;

            env (offer (self, USD(1000), BTC(10)));
            env.close();
            std::uint32_t const selfOfferSeq = env.seq (self) - 1;

            for (auto const& actor : t.actors)
            {
                auto actorOffers = offersOnAccount (env, actor.acct);
                auto const offerCount = std::distance (actorOffers.begin(),
                    std::remove_if (actorOffers.begin(), actorOffers.end(),
                        [] (std::shared_ptr<SLE const>& offer)
                        {
                            return (*offer)[sfTakerGets].signum() == 0;
                        }));
                BEAST_EXPECT (offerCount == actor.offers);

                env.require (balance (actor.acct, actor.xrp));
                env.require (balance (actor.acct, actor.btc));
                env.require (balance (actor.acct, actor.usd));
            }
            env (offer_cancel (leg0, leg0OfferSeq));
            env.close();
            env (offer_cancel (leg1, leg1OfferSeq));
            env.close();
            env (offer_cancel (self, selfOfferSeq));
            env.close();
        }
    }

    void testSelfPayUnlimitedFunds (FeatureBitset features)
    {
        testcase ("Self Pay Unlimited Funds");

        using namespace jtx;

        Env env {*this, features};
        auto const closeTime =
            fix1449Time() +
                100 * env.closed()->info().closeTimeResolution;
        env.close (closeTime);

        auto const gw = Account("gw");
        auto const BTC = gw["BTC"];
        auto const USD = gw["USD"];
        auto const startXrpBalance = XRP (4000000);

        env.fund (startXrpBalance, gw);
        env.close();

        env (rate (gw, 1.25));
        env.close();

        struct Actor
        {
            Account acct;
            int offers;        
            PrettyAmount xrp;  
            PrettyAmount btc;  
            PrettyAmount usd;  
        };
        struct TestData
        {
            std::size_t self;
            std::size_t leg0;
            std::size_t leg1;
            PrettyAmount btcStart;
            std::vector<Actor> actors;
        };

        TestData const takerTests[]
        {
{ 0, 0, 1, BTC( 5), { {"deb", 0, drops(3899999999960), BTC(5), USD(3000)}, {"dan", 0, drops(4099999999970), BTC(0), USD( 750)} } }, 
{ 0, 0, 0, BTC( 5), { {"flo", 0, drops(3999999999950), BTC(5), USD(2000)}                                                      } }  
        };

        TestData const flowTests[]
        {
{ 0, 0, 1, BTC( 5), { {"gay", 1, drops(3949999999960), BTC(5), USD(2500)}, {"gar", 1, drops(4049999999970), BTC(0), USD(1375)} } }, 
{ 0, 0, 0, BTC( 5), { {"hye", 2, drops(3999999999950), BTC(5), USD(2000)}                                                      } }  
        };

        auto const& tests = features[featureFlowCross] ? flowTests : takerTests;

        for (auto const& t : tests)
        {
            Account const& self = t.actors[t.self].acct;
            Account const& leg0 = t.actors[t.leg0].acct;
            Account const& leg1 = t.actors[t.leg1].acct;

            for (auto const& actor : t.actors)
            {
                env.fund (XRP (4000000), actor.acct);
                env.close();

                env (trust (actor.acct, BTC(40)));
                env (trust (actor.acct, USD(8000)));
                env.close();
            }

            env (pay (gw, self, t.btcStart));
            env (pay (gw, self, USD(2000)));
            if (self.id() != leg1.id())
                env (pay (gw, leg1, USD(2000)));
            env.close();

            env (offer (leg0, BTC(10),     XRP(100000), tfPassive));
            env.close();
            std::uint32_t const leg0OfferSeq = env.seq (leg0) - 1;

            env (offer (leg1, XRP(100000), USD(1000),   tfPassive));
            env.close();
            std::uint32_t const leg1OfferSeq = env.seq (leg1) - 1;

            env (offer (self, USD(1000), BTC(10)));
            env.close();
            std::uint32_t const selfOfferSeq = env.seq (self) - 1;

            for (auto const& actor : t.actors)
            {
                auto actorOffers = offersOnAccount (env, actor.acct);
                auto const offerCount = std::distance (actorOffers.begin(),
                    std::remove_if (actorOffers.begin(), actorOffers.end(),
                        [] (std::shared_ptr<SLE const>& offer)
                        {
                            return (*offer)[sfTakerGets].signum() == 0;
                        }));
                BEAST_EXPECT (offerCount == actor.offers);

                env.require (balance (actor.acct, actor.xrp));
                env.require (balance (actor.acct, actor.btc));
                env.require (balance (actor.acct, actor.usd));
            }
            env (offer_cancel (leg0, leg0OfferSeq));
            env.close();
            env (offer_cancel (leg1, leg1OfferSeq));
            env.close();
            env (offer_cancel (self, selfOfferSeq));
            env.close();
        }
    }

    void testRequireAuth (FeatureBitset features)
    {
        testcase ("lsfRequireAuth");

        using namespace jtx;

        Env env {*this, features};
        auto const closeTime =
            fix1449Time() +
                100 * env.closed()->info().closeTimeResolution;
        env.close (closeTime);

        auto const gw = Account("gw");
        auto const alice = Account("alice");
        auto const bob = Account("bob");
        auto const gwUSD = gw["USD"];
        auto const aliceUSD = alice["USD"];
        auto const bobUSD = bob["USD"];

        env.fund (XRP(400000), gw, alice, bob);
        env.close();

        env(fset (gw, asfRequireAuth));
        env.close();

        env (trust (gw, bobUSD(100)), txflags (tfSetfAuth));
        env (trust (bob, gwUSD(100)));
        env (trust (gw, aliceUSD(100)), txflags (tfSetfAuth));
        env (trust (alice, gwUSD(100)));
        env (offer (alice, gwUSD(40), XRP(4000)));
        env.close();

        env.require (offers (alice, 1));
        env.require (balance (alice, gwUSD(0)));

        env (pay(gw, bob, gwUSD(50)));
        env.close();

        env.require (balance (bob, gwUSD(50)));

        env (offer (bob, XRP(4000), gwUSD(40)));
        env.close();

        env.require (offers (alice, 0));
        env.require (balance (alice, gwUSD(40)));

        env.require (offers (bob, 0));
        env.require (balance (bob, gwUSD(10)));
    }

    void testMissingAuth (FeatureBitset features)
    {
        testcase ("Missing Auth");

        using namespace jtx;

        Env env {*this, features};
        auto const closeTime =
            fix1449Time() +
                100 * env.closed()->info().closeTimeResolution;
        env.close (closeTime);

        auto const gw = Account("gw");
        auto const alice = Account("alice");
        auto const bob = Account("bob");
        auto const gwUSD = gw["USD"];
        auto const aliceUSD = alice["USD"];
        auto const bobUSD = bob["USD"];

        env.fund (XRP(400000), gw, alice, bob);
        env.close();

        env (offer (alice, gwUSD(40), XRP(4000)));
        env.close();

        env.require (offers (alice, 1));
        env.require (balance (alice, gwUSD(none)));
        env(fset (gw, asfRequireAuth));
        env.close();

        env (trust (gw, bobUSD(100)), txflags (tfSetfAuth));
        env.close();
        env (trust (bob, gwUSD(100)));
        env.close();

        env (pay(gw, bob, gwUSD(50)));
        env.close();
        env.require (balance (bob, gwUSD(50)));

        env (offer (bob, XRP(4000), gwUSD(40)));
        env.close();
        std::uint32_t const bobOfferSeq = env.seq (bob) - 1;

        bool const flowCross = features[featureFlowCross];

        env.require (offers (alice, 0));
        if (flowCross)
        {
            env.require (balance (alice, gwUSD(none)));
            env.require (offers (bob, 1));
            env.require (balance (bob, gwUSD(50)));
        }
        else
        {
            env.require (balance (alice, gwUSD(40)));
            env.require (offers (bob, 0));
            env.require (balance (bob, gwUSD(10)));

            return;
        }

        env (offer (alice, gwUSD(40), XRP(4000)), ter(tecNO_LINE));
        env.close();

        env.require (offers (alice, 0));
        env.require (balance (alice, gwUSD(none)));

        env.require (offers (bob, 1));
        env.require (balance (bob, gwUSD(50)));

        env (trust (gw, aliceUSD(100)));
        env.close();

        env (offer (alice, gwUSD(40), XRP(4000)), ter(tecNO_AUTH));
        env.close();

        env.require (offers (alice, 0));
        env.require (balance (alice, gwUSD(0)));

        env.require (offers (bob, 1));
        env.require (balance (bob, gwUSD(50)));

        env (offer_cancel (bob, bobOfferSeq));
        env.close();
        env.require (offers (bob, 0));

        env (trust (gw, aliceUSD(100)), txflags (tfSetfAuth));
        env.close();

        env (offer (alice, gwUSD(40), XRP(4000)));
        env.close();

        env.require (offers (alice, 1));

        env (offer (bob, XRP(4000), gwUSD(40)));
        env.close();

        env.require (offers (alice, 0));
        env.require (balance (alice, gwUSD(40)));

        env.require (offers (bob, 0));
        env.require (balance (bob, gwUSD(10)));
    }

    void testRCSmoketest(FeatureBitset features)
    {
        testcase("RippleConnect Smoketest payment flow");
        using namespace jtx;

        Env env {*this, features};
        auto const closeTime =
            fix1449Time() +
                100 * env.closed()->info().closeTimeResolution;
        env.close (closeTime);


        auto const hotUS = Account("hotUS");
        auto const coldUS = Account("coldUS");
        auto const hotEU = Account("hotEU");
        auto const coldEU = Account("coldEU");
        auto const mm = Account("mm");

        auto const USD = coldUS["USD"];
        auto const EUR = coldEU["EUR"];

        env.fund (XRP(100000), hotUS, coldUS, hotEU, coldEU, mm);
        env.close();

        for (auto const& cold : {coldUS, coldEU})
        {
            env(fset (cold, asfRequireAuth));
            env(fset (cold, asfDefaultRipple));
        }
        env.close();

        env (trust(hotUS, USD(10000000)), txflags (tfSetNoRipple));
        env (trust(hotEU, EUR(10000000)), txflags (tfSetNoRipple));
        env (trust(mm, USD(10000000)), txflags (tfSetNoRipple));
        env (trust(mm, EUR(10000000)), txflags (tfSetNoRipple));
        env.close();

        env (trust (coldUS, USD(0), hotUS, tfSetfAuth));
        env (trust (coldEU, EUR(0), hotEU, tfSetfAuth));
        env (trust (coldUS, USD(0), mm, tfSetfAuth));
        env (trust (coldEU, EUR(0), mm, tfSetfAuth));
        env.close();

        env (pay(coldUS, hotUS, USD(5000000)));
        env (pay(coldEU, hotEU, EUR(5000000)));
        env (pay(coldUS, mm, USD(5000000)));
        env (pay(coldEU, mm, EUR(5000000)));
        env.close();

        float const rate = 0.9f; 
        env (offer(mm,  EUR(4000000 * rate), USD(4000000)),
            json(jss::Flags, tfSell));

        float const reverseRate = 1.0f/rate * 1.00101f;
        env (offer(mm,  USD(4000000 * reverseRate), EUR(4000000)),
            json(jss::Flags, tfSell));
        env.close();

        {
            Json::Value jvParams;
            jvParams[jss::destination_account] = coldEU.human();
            jvParams[jss::destination_amount][jss::issuer] = coldEU.human();
            jvParams[jss::destination_amount][jss::currency] = "EUR";
            jvParams[jss::destination_amount][jss::value] = 10;
            jvParams[jss::source_account] = hotUS.human();

            Json::Value const jrr {env.rpc(
                "json", "ripple_path_find", to_string(jvParams))[jss::result]};

            BEAST_EXPECT(jrr[jss::status] == "success");
            BEAST_EXPECT(
                jrr[jss::alternatives].isArray() &&
                jrr[jss::alternatives].size() > 0);
        }
        env (pay (hotUS, coldEU, EUR(10)), sendmax (USD(11.1223326)));
    }

    void testSelfAuth (FeatureBitset features)
    {
        testcase ("Self Auth");

        using namespace jtx;

        Env env {*this, features};
        auto const closeTime =
            fix1449Time() +
                100 * env.closed()->info().closeTimeResolution;
        env.close (closeTime);

        auto const gw = Account("gw");
        auto const alice = Account("alice");
        auto const gwUSD = gw["USD"];
        auto const aliceUSD = alice["USD"];

        env.fund (XRP(400000), gw, alice);
        env.close();

        env (offer (gw, gwUSD(40), XRP(4000)));
        env.close();
        std::uint32_t const gwOfferSeq = env.seq (gw) - 1;
        env.require (offers (gw, 1));

        env(fset (gw, asfRequireAuth), ter (tecOWNERS));
        env.close();

        env (offer_cancel (gw, gwOfferSeq));
        env.close();
        env.require (offers (gw, 0));

        env(fset (gw, asfRequireAuth));
        env.close();

        bool const preauth = features[featureDepositPreauth];

        env (offer (gw, gwUSD(40), XRP(4000)),
            ter (preauth ? TER {tesSUCCESS} : TER {tecNO_LINE}));
        env.close();

        env.require (offers (gw, preauth ? 1 : 0));

        if (!preauth)
            return;

        env (trust (gw, aliceUSD(100)), txflags (tfSetfAuth));
        env (trust (alice, gwUSD(100)));
        env.close();

        env (pay(gw, alice, gwUSD(50)));
        env.close();

        env.require (balance (alice, gwUSD(50)));

        env (offer (alice, XRP(4000), gwUSD(40)));
        env.close();

        env.require (offers (alice, 0));
        env.require (balance (alice, gwUSD(10)));

        env.require (offers (gw, 0));
    }

    void testTickSize (FeatureBitset features)
    {
        testcase ("Tick Size");

        using namespace jtx;

        BEAST_EXPECT(features[featureTickSize]);

        {
            Env env{*this, features - featureTickSize};
            auto const gw = Account {"gateway"};
            env.fund (XRP(10000), gw);

            auto txn = noop(gw);
            txn[sfTickSize.fieldName] = 0;
            env(txn, ter(temDISABLED));
        }

        {
            Env env {*this, features};
            auto const gw = Account {"gateway"};
            env.fund (XRP(10000), gw);

            auto txn = noop(gw);
            txn[sfTickSize.fieldName] = Quality::minTickSize - 1;
            env(txn, ter (temBAD_TICK_SIZE));

            txn[sfTickSize.fieldName] = Quality::minTickSize;
            env(txn);
            BEAST_EXPECT ((*env.le(gw))[sfTickSize]
                == Quality::minTickSize);

            txn = noop (gw);
            txn[sfTickSize.fieldName] = Quality::maxTickSize;
            env(txn);
            BEAST_EXPECT (! env.le(gw)->isFieldPresent (sfTickSize));

            txn = noop (gw);
            txn[sfTickSize.fieldName] = Quality::maxTickSize - 1;
            env(txn);
            BEAST_EXPECT ((*env.le(gw))[sfTickSize]
                == Quality::maxTickSize - 1);

            txn = noop (gw);
            txn[sfTickSize.fieldName] = Quality::maxTickSize + 1;
            env(txn, ter (temBAD_TICK_SIZE));

            txn[sfTickSize.fieldName] = 0;
            env(txn);
            BEAST_EXPECT (! env.le(gw)->isFieldPresent (sfTickSize));
        }

        Env env {*this, features};
        auto const gw = Account {"gateway"};
        auto const alice = Account {"alice"};
        auto const XTS = gw["XTS"];
        auto const XXX = gw["XXX"];

        env.fund (XRP (10000), gw, alice);

        {
            auto txn = noop(gw);
            txn[sfTickSize.fieldName] = 5;
            env(txn);
            BEAST_EXPECT ((*env.le(gw))[sfTickSize] == 5);
        }

        env (trust (alice, XTS (1000)));
        env (trust (alice, XXX (1000)));

        env (pay (gw, alice, alice["XTS"] (100)));
        env (pay (gw, alice, alice["XXX"] (100)));

        env (offer (alice, XTS (10), XXX (30)));
        env (offer (alice, XTS (30), XXX (10)));
        env (offer (alice, XTS (10), XXX (30)),
            json(jss::Flags, tfSell));
        env (offer (alice, XTS (30), XXX (10)),
            json(jss::Flags, tfSell));

        std::map <std::uint32_t, std::pair<STAmount, STAmount>> offers;
        forEachItem (*env.current(), alice,
            [&](std::shared_ptr<SLE const> const& sle)
        {
            if (sle->getType() == ltOFFER)
                offers.emplace((*sle)[sfSequence],
                    std::make_pair((*sle)[sfTakerPays],
                        (*sle)[sfTakerGets]));
        });

        auto it = offers.begin();
        BEAST_EXPECT (it != offers.end());
        BEAST_EXPECT (it->second.first == XTS(10) &&
            it->second.second < XXX(30) &&
            it->second.second > XXX(29.9994));

        ++it;
        BEAST_EXPECT (it != offers.end());
        BEAST_EXPECT (it->second.first == XTS(30) &&
            it->second.second == XXX(10));

        ++it;
        BEAST_EXPECT (it != offers.end());
        BEAST_EXPECT (it->second.first == XTS(10.0002) &&
            it->second.second == XXX(30));

        ++it;
        BEAST_EXPECT (it != offers.end());
        BEAST_EXPECT (it->second.first == XTS(30) &&
            it->second.second == XXX(10));

        BEAST_EXPECT (++it == offers.end());
    }

    void testAll(FeatureBitset features)
    {
        testCanceledOffer(features);
        testRmFundedOffer(features);
        testTinyPayment(features);
        testXRPTinyPayment(features);
        testEnforceNoRipple(features);
        testInsufficientReserve(features);
        testFillModes(features);
        testMalformed(features);
        testExpiration(features);
        testUnfundedCross(features);
        testSelfCross(false, features);
        testSelfCross(true, features);
        testNegativeBalance(features);
        testOfferCrossWithXRP(true, features);
        testOfferCrossWithXRP(false, features);
        testOfferCrossWithLimitOverride(features);
        testOfferAcceptThenCancel(features);
        testOfferCancelPastAndFuture(features);
        testCurrencyConversionEntire(features);
        testCurrencyConversionIntoDebt(features);
        testCurrencyConversionInParts(features);
        testCrossCurrencyStartXRP(features);
        testCrossCurrencyEndXRP(features);
        testCrossCurrencyBridged(features);
        testBridgedSecondLegDry(features);
        testOfferFeesConsumeFunds(features);
        testOfferCreateThenCross(features);
        testSellFlagBasic(features);
        testSellFlagExceedLimit(features);
        testGatewayCrossCurrency(features);
        testPartialCross (features);
        testXRPDirectCross (features);
        testDirectCross (features);
        testBridgedCross (features);
        testSellOffer (features);
        testSellWithFillOrKill (features);
        testTransferRateOffer(features);
        testSelfCrossOffer (features);
        testSelfIssueOffer (features);
        testBadPathAssert (features);
        testDirectToDirectPath (features);
        testSelfCrossLowQualityOffer (features);
        testOfferInScaling (features);
        testOfferInScalingWithXferRate (features);
        testOfferThresholdWithReducedFunds (features);
        testTinyOffer (features);
        testSelfPayXferFeeOffer (features);
        testSelfPayUnlimitedFunds (features);
        testRequireAuth (features);
        testMissingAuth (features);
        testRCSmoketest (features);
        testSelfAuth (features);
        testTickSize (features | featureTickSize);
    }

    void run () override
    {
        using namespace jtx;
        FeatureBitset const all{supported_amendments()};
        FeatureBitset const f1373{fix1373};
        FeatureBitset const flowCross{featureFlowCross};
        FeatureBitset const takerDryOffer{fixTakerDryOfferRemoval};

        testAll(all - f1373             - takerDryOffer);
        testAll(all         - flowCross - takerDryOffer);
        testAll(all         - flowCross                );
        testAll(all                                    );
    }
};

class Offer_manual_test : public Offer_test
{
    void run() override
    {
        using namespace jtx;
        FeatureBitset const all{supported_amendments()};
        FeatureBitset const flow{featureFlow};
        FeatureBitset const f1373{fix1373};
        FeatureBitset const flowCross{featureFlowCross};
        FeatureBitset const f1513{fix1513};
        FeatureBitset const takerDryOffer{fixTakerDryOfferRemoval};

        testAll(all                - flow - f1373 - flowCross - f1513);
        testAll(all                - flow - f1373 - flowCross        );
        testAll(all                - flow - f1373             - f1513);
        testAll(all                - flow - f1373                    );
        testAll(all                       - f1373 - flowCross - f1513);
        testAll(all                       - f1373 - flowCross        );
        testAll(all                       - f1373             - f1513);
        testAll(all                       - f1373                    );
        testAll(all                               - flowCross - f1513);
        testAll(all                               - flowCross        );
        testAll(all                                           - f1513);
        testAll(all                                                  );

        testAll(all                - flow - f1373 - flowCross - takerDryOffer);
        testAll(all                - flow - f1373             - takerDryOffer);
        testAll(all                       - f1373 - flowCross - takerDryOffer);
    }
};

BEAST_DEFINE_TESTSUITE_PRIO (Offer, tx, ripple, 4);
BEAST_DEFINE_TESTSUITE_MANUAL_PRIO (Offer_manual, tx, ripple, 20);

}  
}  
























