

#include <test/jtx.h>
#include <ripple/beast/unit_test.h>
#include <ripple/protocol/jss.h>

namespace ripple {

class AccountCurrencies_test : public beast::unit_test::suite
{
    void
    testBadInput ()
    {
        testcase ("Bad input to account_currencies");

        using namespace test::jtx;
        Env env {*this};

        auto const alice = Account {"alice"};
        env.fund (XRP(10000), alice);
        env.close ();

        { 
            Json::Value params;
            params[jss::ledger_hash] = 1;
            auto const result = env.rpc ("json", "account_currencies",
                boost::lexical_cast<std::string>(params)) [jss::result];
            BEAST_EXPECT (result[jss::error] == "invalidParams");
            BEAST_EXPECT (result[jss::error_message] ==
                "ledgerHashNotString");
        }

        { 
            auto const result =
                env.rpc ("json", "account_currencies", "{}") [jss::result];
            BEAST_EXPECT (result[jss::error] == "invalidParams");
            BEAST_EXPECT (result[jss::error_message] ==
                "Missing field 'account'.");
        }

        { 
            Json::Value params;
            params[jss::account] = "llIIOO"; 
            params[jss::strict] = true;
            auto const result = env.rpc ("json", "account_currencies",
                boost::lexical_cast<std::string>(params)) [jss::result];
            BEAST_EXPECT (result[jss::error] == "actMalformed");
            BEAST_EXPECT (result[jss::error_message] ==
                "Account malformed.");
        }

        { 
            Json::Value params;
            params[jss::account] = base58EncodeTokenBitcoin (
                TokenType::AccountID, alice.id().data(), alice.id().size());
            params[jss::strict] = true;
            auto const result = env.rpc ("json", "account_currencies",
                boost::lexical_cast<std::string>(params)) [jss::result];
            BEAST_EXPECT (result[jss::error] == "actBitcoin");
            BEAST_EXPECT (result[jss::error_message] ==
                "Account is bitcoin address.");
        }

        { 
            Json::Value params;
            params[jss::account] = Account{"bob"}.human();
            auto const result = env.rpc ("json", "account_currencies",
                boost::lexical_cast<std::string>(params)) [jss::result];
            BEAST_EXPECT (result[jss::error] == "actNotFound");
            BEAST_EXPECT (result[jss::error_message] ==
                "Account not found.");
        }
    }

    void
    testBasic ()
    {
        testcase ("Basic request for account_currencies");

        using namespace test::jtx;
        Env env {*this};

        auto const alice = Account {"alice"};
        auto const gw = Account {"gateway"};
        env.fund (XRP(10000), alice, gw);
        char currencySuffix {'A'};
        std::vector<boost::optional<IOU>> gwCurrencies (26); 
        std::generate (gwCurrencies.begin(), gwCurrencies.end(),
            [&]()
            {
                auto gwc = gw[std::string("US") + currencySuffix++];
                env (trust (alice, gwc (100)));
                return gwc;
            });
        env.close ();

        Json::Value params;
        params[jss::account] = alice.human();
        auto result = env.rpc ("json", "account_currencies",
            boost::lexical_cast<std::string>(params)) [jss::result];

        auto arrayCheck =
            [&result] (
                Json::StaticString const& fld,
                std::vector<boost::optional<IOU>> const& expected) -> bool
            {
                bool stat =
                    result.isMember (fld) &&
                    result[fld].isArray() &&
                    result[fld].size() == expected.size();
                for (size_t i = 0; stat && i < expected.size(); ++i)
                {
                    Currency foo;
                    stat &= (
                        to_string(expected[i].value().currency) ==
                        result[fld][i].asString()
                    );
                }
                return stat;
            };

        BEAST_EXPECT (arrayCheck (jss::receive_currencies, gwCurrencies));
        BEAST_EXPECT (arrayCheck (jss::send_currencies, {}));

        for (auto const& c : gwCurrencies)
            env (pay (gw, alice, c.value()(50)));

        result = env.rpc ("json", "account_currencies",
            boost::lexical_cast<std::string>(params)) [jss::result];
        BEAST_EXPECT (arrayCheck (jss::receive_currencies, gwCurrencies));
        BEAST_EXPECT (arrayCheck (jss::send_currencies, gwCurrencies));

        env(trust(alice, gw["USD"](100), tfSetFreeze));
        result = env.rpc ("account_lines", alice.human());
        for (auto const l : result[jss::lines])
            BEAST_EXPECT(
                l[jss::freeze].asBool() == (l[jss::currency] == "USD"));
        result = env.rpc ("json", "account_currencies",
            boost::lexical_cast<std::string>(params)) [jss::result];
        BEAST_EXPECT (arrayCheck (jss::receive_currencies, gwCurrencies));
        BEAST_EXPECT (arrayCheck (jss::send_currencies, gwCurrencies));
        env(trust(alice, gw["USD"](100), tfClearFreeze));

        env (pay (gw, alice, gw["USA"](50)));
        result = env.rpc ("json", "account_currencies",
            boost::lexical_cast<std::string>(params)) [jss::result];
        decltype(gwCurrencies) gwCurrenciesNoUSA (gwCurrencies.begin() + 1,
            gwCurrencies.end());
        BEAST_EXPECT (arrayCheck (jss::receive_currencies, gwCurrenciesNoUSA));
        BEAST_EXPECT (arrayCheck (jss::send_currencies, gwCurrencies));

        env (trust (gw, alice["USA"] (100)));
        env (pay (alice, gw, alice["USA"](200)));
        result = env.rpc ("json", "account_currencies",
            boost::lexical_cast<std::string>(params)) [jss::result];
        BEAST_EXPECT (arrayCheck (jss::receive_currencies, gwCurrencies));
        BEAST_EXPECT (arrayCheck (jss::send_currencies, gwCurrenciesNoUSA));
    }

public:
    void run () override
    {
        testBadInput ();
        testBasic ();
    }
};

BEAST_DEFINE_TESTSUITE(AccountCurrencies,app,ripple);

} 

























