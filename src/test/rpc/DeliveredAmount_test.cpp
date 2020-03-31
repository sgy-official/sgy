

#include <ripple/beast/unit_test.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/jss.h>
#include <test/jtx.h>
#include <test/jtx/WSClient.h>

namespace ripple {
namespace test {

class CheckDeliveredAmount
{
    bool afterSwitchTime_;
    int numExpectedAvailable_ = 0;
    int numExpectedSetUnavailable_ = 0;
    int numExpectedNotSet_ = 0;

    void
    adjCounters(bool success, bool partial)
    {
        if (!success)
        {
            ++numExpectedNotSet_;
            return;
        }
        if (!afterSwitchTime_)
        {
            if (partial)
                ++numExpectedAvailable_;
            else
                ++numExpectedSetUnavailable_;
            return;
        }
        ++numExpectedAvailable_;
    }

public:
    explicit CheckDeliveredAmount(bool afterSwitchTime)
        : afterSwitchTime_(afterSwitchTime)
    {
    }

    void
    adjCountersSuccess()
    {
        adjCounters(true, false);
    }

    void
    adjCountersFail()
    {
        adjCounters(false, false);
    }
    void
    adjCountersPartialPayment()
    {
        adjCounters(true, true);
    }

    bool
    checkExpectedCounters() const
    {
        return !numExpectedAvailable_ && !numExpectedNotSet_ &&
            !numExpectedSetUnavailable_;
    }

    bool
    checkTxn(Json::Value const& t, Json::Value const& metaData)
    {
        if (t[jss::TransactionType].asString() != jss::Payment)
            return true;

        bool isSet = metaData.isMember(jss::delivered_amount);
        bool isSetUnavailable = false;
        bool isSetAvailable = false;
        if (isSet)
        {
            if (metaData[jss::delivered_amount] != "unavailable")
                isSetAvailable = true;
            else
                isSetUnavailable = true;
        }
        if (isSetAvailable)
            --numExpectedAvailable_;
        else if (isSetUnavailable)
            --numExpectedSetUnavailable_;
        else if (!isSet)
            --numExpectedNotSet_;

        if (isSet)
        {
            if (metaData.isMember(sfDeliveredAmount.jsonName))
            {
                if (metaData[jss::delivered_amount] !=
                    metaData[sfDeliveredAmount.jsonName])
                    return false;
            }
            else
            {
                if (afterSwitchTime_)
                {
                    if (metaData[jss::delivered_amount] != t[jss::Amount])
                        return false;
                }
                else
                {
                    if (metaData[jss::delivered_amount] != "unavailable")
                        return false;
                }
            }
        }

        if (metaData[sfTransactionResult.jsonName] != "tesSUCCESS")
        {
            if (isSet)
                return false;
        }
        else
        {
            if (afterSwitchTime_)
            {
                if (!isSetAvailable)
                    return false;
            }
            else
            {
                if (metaData.isMember(sfDeliveredAmount.jsonName))
                {
                    if (!isSetAvailable)
                        return false;
                }
                else
                {
                    if (!isSetUnavailable)
                        return false;
                }
            }
        }
        return true;
    }
};

class DeliveredAmount_test : public beast::unit_test::suite
{
    void
    testAccountDeliveredAmountSubscribe()
    {
        testcase("Ledger Request Subscribe DeliveredAmount");

        using namespace test::jtx;
        using namespace std::chrono_literals;

        Account const alice("alice");
        Account const bob("bob");
        Account const carol("carol");
        auto const gw = Account("gateway");
        auto const USD = gw["USD"];

        for (bool const afterSwitchTime : {true, false})
        {
            Env env{*this};
            env.fund(XRP(10000), alice, bob, carol, gw);
            env.trust(USD(1000), alice, bob, carol);
            if (afterSwitchTime)
                env.close(NetClock::time_point{446000000s});
            else
                env.close();

            CheckDeliveredAmount checkDeliveredAmount{afterSwitchTime};
            {

                env(pay(gw, alice, USD(50)));
                checkDeliveredAmount.adjCountersSuccess();
                env(pay(gw, alice, XRP(50)));
                checkDeliveredAmount.adjCountersSuccess();

                env(pay(gw, bob, USD(9999999)), txflags(tfPartialPayment));
                checkDeliveredAmount.adjCountersPartialPayment();
                env.require(balance(bob, USD(1000)));

                env(pay(bob, carol, USD(9999999)), ter(tecPATH_PARTIAL));
                checkDeliveredAmount.adjCountersFail();
                env.require(balance(carol, USD(0)));
            }

            auto wsc = makeWSClient(env.app().config());

            {
                Json::Value stream;
                stream[jss::streams] = Json::arrayValue;
                stream[jss::streams].append("ledger");
                stream[jss::accounts] = Json::arrayValue;
                stream[jss::accounts].append(toBase58(alice.id()));
                stream[jss::accounts].append(toBase58(bob.id()));
                stream[jss::accounts].append(toBase58(carol.id()));
                auto jv = wsc->invoke("subscribe", stream);
                if (wsc->version() == 2)
                {
                    BEAST_EXPECT(
                        jv.isMember(jss::jsonrpc) && jv[jss::jsonrpc] == "2.0");
                    BEAST_EXPECT(
                        jv.isMember(jss::ripplerpc) &&
                        jv[jss::ripplerpc] == "2.0");
                    BEAST_EXPECT(jv.isMember(jss::id) && jv[jss::id] == 5);
                }
                BEAST_EXPECT(jv[jss::result][jss::ledger_index] == 3);
            }
            {
                env.close();
                while (true)
                {
                    auto const r = wsc->findMsg(1s, [&](auto const& jv) {
                        return jv[jss::ledger_index] == 4;
                    });
                    if (!r)
                        break;

                    if (!r->isMember(jss::transaction))
                        continue;

                    BEAST_EXPECT(checkDeliveredAmount.checkTxn(
                        (*r)[jss::transaction], (*r)[jss::meta]));
                }
            }
            BEAST_EXPECT(checkDeliveredAmount.checkExpectedCounters());
        }
    }
    void
    testTxDeliveredAmountRPC()
    {
        testcase("Ledger Request RPC DeliveredAmount");

        using namespace test::jtx;
        using namespace std::chrono_literals;

        Account const alice("alice");
        Account const bob("bob");
        Account const carol("carol");
        auto const gw = Account("gateway");
        auto const USD = gw["USD"];

        for (bool const afterSwitchTime : {true, false})
        {
            Env env{*this};
            env.fund(XRP(10000), alice, bob, carol, gw);
            env.trust(USD(1000), alice, bob, carol);
            if (afterSwitchTime)
                env.close(NetClock::time_point{446000000s});
            else
                env.close();

            CheckDeliveredAmount checkDeliveredAmount{afterSwitchTime};
            env(pay(gw, alice, USD(50)));
            checkDeliveredAmount.adjCountersSuccess();
            env(pay(gw, alice, XRP(50)));
            checkDeliveredAmount.adjCountersSuccess();

            env(pay(gw, bob, USD(9999999)), txflags(tfPartialPayment));
            checkDeliveredAmount.adjCountersPartialPayment();
            env.require(balance(bob, USD(1000)));

            env(pay(gw, carol, USD(9999999)), ter(tecPATH_PARTIAL));
            checkDeliveredAmount.adjCountersFail();
            env.require(balance(carol, USD(0)));

            env.close();
            std::string index;
            Json::Value jvParams;
            jvParams[jss::ledger_index] = 4u;
            jvParams[jss::transactions] = true;
            jvParams[jss::expand] = true;
            auto const jtxn = env.rpc(
                "json",
                "ledger",
                to_string(
                    jvParams))[jss::result][jss::ledger][jss::transactions];
            for (auto const& t : jtxn)
                BEAST_EXPECT(
                    checkDeliveredAmount.checkTxn(t, t[jss::metaData]));
            BEAST_EXPECT(checkDeliveredAmount.checkExpectedCounters());
        }
    }

public:
    void
    run() override
    {
        testTxDeliveredAmountRPC();
        testAccountDeliveredAmountSubscribe();
    }
};

BEAST_DEFINE_TESTSUITE(DeliveredAmount, app, ripple);

}  
}  
























