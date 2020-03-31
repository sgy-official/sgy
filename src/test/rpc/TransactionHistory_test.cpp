

#include <test/jtx.h>
#include <test/jtx/Env.h>
#include <test/jtx/envconfig.h>
#include <ripple/protocol/jss.h>
#include <boost/container/static_vector.hpp>
#include <algorithm>

namespace ripple {

class TransactionHistory_test : public beast::unit_test::suite
{
    void
    testBadInput()
    {
        testcase("Invalid request params");
        using namespace test::jtx;
        Env env {*this, envconfig(no_admin)};

        {
            auto const result = env.client()
                .invoke("tx_history", {})[jss::result];
            BEAST_EXPECT(result[jss::error] == "invalidParams");
            BEAST_EXPECT(result[jss::status] == "error");
        }

        {
            Json::Value params {Json::objectValue};
            params[jss::start] = 10001; 
            auto const result = env.client()
                .invoke("tx_history", params)[jss::result];
            BEAST_EXPECT(result[jss::error] == "noPermission");
            BEAST_EXPECT(result[jss::status] == "error");
        }
    }

    void testRequest()
    {
        testcase("Basic request");
        using namespace test::jtx;
        Env env {*this};

        size_t const numAccounts = 20;
        boost::container::static_vector<Account, numAccounts> accounts;
        for(size_t i = 0; i<numAccounts; ++i)
        {
            accounts.emplace_back("A" + std::to_string(i));
            auto const& acct=accounts.back();
            env.fund(XRP(10000), acct);
            env.close();
            if(i > 0)
            {
                auto const& prev=accounts[i-1];
                env.trust(acct["USD"](1000), prev);
                env(pay(acct, prev, acct["USD"](5)));
            }
            env(offer(acct, XRP(100), acct["USD"](1)));
            env.close();

            Json::Value params {Json::objectValue};
            params[jss::start] = 0;
            auto result =
                env.client().invoke("tx_history", params)[jss::result];
            if(! BEAST_EXPECT(result[jss::txs].isArray() &&
                    result[jss::txs].size() > 0))
                return;

            bool const txFound = [&] {
                auto const toFind = env.tx()->getJson(JsonOptions::none);
                for (auto tx : result[jss::txs])
                {
                    tx.removeMember(jss::inLedger);
                    tx.removeMember(jss::ledger_index);
                    if (toFind == tx)
                        return true;
                }
                return false;
            }();
            BEAST_EXPECT(txFound);
        }

        unsigned int start = 0;
        unsigned int total = 0;
        std::unordered_map<std::string, unsigned> typeCounts;
        while(start < 120)
        {
            Json::Value params {Json::objectValue};
            params[jss::start] = start;
            auto result =
                env.client().invoke("tx_history", params)[jss::result];
            if(! BEAST_EXPECT(result[jss::txs].isArray() &&
                    result[jss::txs].size() > 0))
                break;
            total += result[jss::txs].size();
            start += 20;
            for (auto const& t : result[jss::txs])
            {
                typeCounts[t[sfTransactionType.fieldName].asString()]++;
            }
        }
        BEAST_EXPECT(total == 117);
        BEAST_EXPECT(typeCounts[jss::AccountSet.c_str()] == 20);
        BEAST_EXPECT(typeCounts[jss::TrustSet.c_str()] == 19);
        BEAST_EXPECT(typeCounts[jss::Payment.c_str()] == 58);
        BEAST_EXPECT(typeCounts[jss::OfferCreate.c_str()] == 20);

        {
            Json::Value params {Json::objectValue};
            params[jss::start] = 10000; 
            auto const result = env.client()
                .invoke("tx_history", params)[jss::result];
            BEAST_EXPECT(result[jss::status] == "success");
            BEAST_EXPECT(result[jss::index] == 10000);
        }
    }

public:
    void run () override
    {
        testBadInput();
        testRequest();
    }
};

BEAST_DEFINE_TESTSUITE (TransactionHistory, rpc, ripple);

}  
























