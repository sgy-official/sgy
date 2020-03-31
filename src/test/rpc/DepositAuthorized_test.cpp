

#include <ripple/protocol/jss.h>
#include <test/jtx.h>

namespace ripple {
namespace test {

class DepositAuthorized_test : public beast::unit_test::suite
{
public:
    static Json::Value depositAuthArgs (
        jtx::Account const& source,
        jtx::Account const& dest, std::string const& ledger = "")
    {
        Json::Value args {Json::objectValue};
        args[jss::source_account] = source.human();
        args[jss::destination_account] = dest.human();
        if (! ledger.empty())
            args[jss::ledger_index] = ledger;
        return args;
    }

    void validateDepositAuthResult (Json::Value const& result, bool authorized)
    {
        Json::Value const& results {result[jss::result]};
        BEAST_EXPECT (results[jss::deposit_authorized] == authorized);
        BEAST_EXPECT (results[jss::status] == jss::success);
    }

    void testValid()
    {
        using namespace jtx;
        Account const alice {"alice"};
        Account const becky {"becky"};
        Account const carol {"carol"};

        Env env(*this);
        env.fund(XRP(1000), alice, becky, carol);
        env.close();

        validateDepositAuthResult (env.rpc ("json", "deposit_authorized",
            depositAuthArgs (
                becky, becky, "validated").toStyledString()), true);

        validateDepositAuthResult (env.rpc ("json", "deposit_authorized",
            depositAuthArgs (
                alice, becky, "validated").toStyledString()), true);

        env (fset (becky, asfDepositAuth));

        validateDepositAuthResult (env.rpc ("json", "deposit_authorized",
            depositAuthArgs (alice, becky).toStyledString()), false);
        env.close();

        validateDepositAuthResult (env.rpc ("json", "deposit_authorized",
            depositAuthArgs (
                becky, becky, "validated").toStyledString()), true);

        validateDepositAuthResult (env.rpc ("json", "deposit_authorized",
            depositAuthArgs (
                becky, alice, "current").toStyledString()), true);

        env (deposit::auth (becky, alice));
        env.close();

        validateDepositAuthResult (env.rpc ("json", "deposit_authorized",
            depositAuthArgs (alice, becky, "closed").toStyledString()), true);

        validateDepositAuthResult (env.rpc ("json", "deposit_authorized",
            depositAuthArgs (carol, becky).toStyledString()), false);

        env (fclear (becky, asfDepositAuth));
        env.close();

        validateDepositAuthResult (env.rpc ("json", "deposit_authorized",
            depositAuthArgs (carol, becky).toStyledString()), true);

        validateDepositAuthResult (env.rpc ("json", "deposit_authorized",
            depositAuthArgs (alice, becky).toStyledString()), true);
    }

    void testErrors()
    {
        using namespace jtx;
        Account const alice {"alice"};
        Account const becky {"becky"};

        auto verifyErr = [this] (
            Json::Value const& result, char const* error, char const* errorMsg)
        {
            BEAST_EXPECT (result[jss::result][jss::status] == jss::error);
            BEAST_EXPECT (result[jss::result][jss::error] == error);
            BEAST_EXPECT (result[jss::result][jss::error_message] == errorMsg);
        };

        Env env(*this);
        {
            Json::Value args {depositAuthArgs (alice, becky)};
            args.removeMember (jss::source_account);
            Json::Value const result {env.rpc (
                "json", "deposit_authorized", args.toStyledString())};
            verifyErr (result, "invalidParams",
                "Missing field 'source_account'.");
        }
        {
            Json::Value args {depositAuthArgs (alice, becky)};
            args[jss::source_account] = 7.3;
            Json::Value const result {env.rpc (
                "json", "deposit_authorized", args.toStyledString())};
            verifyErr (result, "invalidParams",
                "Invalid field 'source_account', not a string.");
        }
        {
            Json::Value args {depositAuthArgs (alice, becky)};
            args[jss::source_account] = "rG1QQv2nh2gr7RCZ!P8YYcBUKCCN633jCn";
            Json::Value const result {env.rpc (
                "json", "deposit_authorized", args.toStyledString())};
            verifyErr (result, "actMalformed", "Account malformed.");
        }
        {
            Json::Value args {depositAuthArgs (alice, becky)};
            args.removeMember (jss::destination_account);
            Json::Value const result {env.rpc (
                "json", "deposit_authorized", args.toStyledString())};
            verifyErr (result, "invalidParams",
                "Missing field 'destination_account'.");
        }
        {
            Json::Value args {depositAuthArgs (alice, becky)};
            args[jss::destination_account] = 7.3;
            Json::Value const result {env.rpc (
                "json", "deposit_authorized", args.toStyledString())};
            verifyErr (result, "invalidParams",
                "Invalid field 'destination_account', not a string.");
        }
        {
            Json::Value args {depositAuthArgs (alice, becky)};
            args[jss::destination_account] =
                "rP6P9ypfAmc!pw8SZHNwM4nvZHFXDraQas";
            Json::Value const result {env.rpc (
                "json", "deposit_authorized", args.toStyledString())};
            verifyErr (result, "actMalformed", "Account malformed.");
        }
        {
            Json::Value args {depositAuthArgs (alice, becky, "17")};
            Json::Value const result {env.rpc (
                "json", "deposit_authorized", args.toStyledString())};
            verifyErr (result, "invalidParams", "ledgerIndexMalformed");
        }
        {
            Json::Value args {depositAuthArgs (alice, becky)};
            args[jss::ledger_index] = 17;
            Json::Value const result {env.rpc (
                "json", "deposit_authorized", args.toStyledString())};
            verifyErr (result, "lgrNotFound", "ledgerNotFound");
        }
        {
            Json::Value args {depositAuthArgs (alice, becky)};
            Json::Value const result {env.rpc (
                "json", "deposit_authorized", args.toStyledString())};
            verifyErr (result, "srcActNotFound",
                "Source account not found.");
        }
        env.fund(XRP(1000), alice);
        env.close();
        {
            Json::Value args {depositAuthArgs (alice, becky)};
            Json::Value const result {env.rpc (
                "json", "deposit_authorized", args.toStyledString())};
            verifyErr (result, "dstActNotFound",
                "Destination account not found.");
        }
        env.fund(XRP(1000), becky);
        env.close();
        {
            Json::Value args {depositAuthArgs (alice, becky)};
            Json::Value const result {env.rpc (
                "json", "deposit_authorized", args.toStyledString())};
            validateDepositAuthResult (result, true);
        }
    }

    void run() override
    {
        testValid();
        testErrors();
    }
};

BEAST_DEFINE_TESTSUITE(DepositAuthorized,app,ripple);

}
}

























