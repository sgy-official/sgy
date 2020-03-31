

#include <test/jtx.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/jss.h>

namespace ripple {
namespace test {

struct SetAuth_test : public beast::unit_test::suite
{
    static
    Json::Value
    auth (jtx::Account const& account,
        jtx::Account const& dest,
            std::string const& currency)
    {
        using namespace jtx;
        Json::Value jv;
        jv[jss::Account] = account.human();
        jv[jss::LimitAmount] = STAmount(
            { to_currency(currency), dest }).getJson(JsonOptions::none);
        jv[jss::TransactionType] = jss::TrustSet;
        jv[jss::Flags] = tfSetfAuth;
        return jv;
    }

    void testAuth(FeatureBitset features)
    {
        BEAST_EXPECT(!features[featureTrustSetAuth]);

        using namespace jtx;
        auto const gw = Account("gw");
        auto const USD = gw["USD"];
        {
            Env env(*this, features);
            env.fund(XRP(100000), "alice", gw);
            env(fset(gw, asfRequireAuth));
            env(auth(gw, "alice", "USD"),       ter(tecNO_LINE_REDUNDANT));
        }
        {
            Env env(*this, features | featureTrustSetAuth);

            env.fund(XRP(100000), "alice", "bob", gw);
            env(fset(gw, asfRequireAuth));
            env(auth(gw, "alice", "USD"));
            BEAST_EXPECT(env.le(
                keylet::line(Account("alice").id(),
                    gw.id(), USD.currency)));
            env(trust("alice", USD(1000)));
            env(trust("bob", USD(1000)));
            env(pay(gw, "alice", USD(100)));
            env(pay(gw, "bob", USD(100)),       ter(tecPATH_DRY)); 
            env(pay("alice", "bob", USD(50)),   ter(tecPATH_DRY)); 
        }
    }

    void run() override
    {
        using namespace jtx;
        auto const sa = supported_amendments();
        testAuth(sa - featureTrustSetAuth - featureFlow - fix1373 - featureFlowCross);
        testAuth(sa - featureTrustSetAuth - fix1373 - featureFlowCross);
        testAuth(sa - featureTrustSetAuth - featureFlowCross);
        testAuth(sa - featureTrustSetAuth);
    }
};

BEAST_DEFINE_TESTSUITE(SetAuth,test,ripple);

} 
} 
























