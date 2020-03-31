

#include <test/jtx.h>
#include <test/jtx/Env.h>
#include <ripple/protocol/jss.h>

namespace ripple {

class TransactionEntry_test : public beast::unit_test::suite
{
    void
    testBadInput()
    {
        testcase("Invalid request params");
        using namespace test::jtx;
        Env env {*this};

        {
            auto const result = env.client()
                .invoke("transaction_entry", {})[jss::result];
            BEAST_EXPECT(result[jss::error] == "fieldNotFoundTransaction");
            BEAST_EXPECT(result[jss::status] == "error");
        }

        {
            Json::Value params {Json::objectValue};
            params[jss::ledger] = 20;
            auto const result = env.client()
                .invoke("transaction_entry", params)[jss::result];
            BEAST_EXPECT(result[jss::error] == "lgrNotFound");
            BEAST_EXPECT(result[jss::status] == "error");
        }

        {
            Json::Value params {Json::objectValue};
            params[jss::ledger] = "current";
            params[jss::tx_hash] = "DEADBEEF";
            auto const result = env.client()
                .invoke("transaction_entry", params)[jss::result];
            BEAST_EXPECT(result[jss::error] == "notYetImplemented");
            BEAST_EXPECT(result[jss::status] == "error");
        }

        {
            Json::Value params {Json::objectValue};
            params[jss::ledger] = "closed";
            params[jss::tx_hash] = "DEADBEEF";
            auto const result = env.client()
                .invoke("transaction_entry", params)[jss::result];
            BEAST_EXPECT(! result[jss::ledger_hash].asString().empty());
            BEAST_EXPECT(result[jss::error] == "transactionNotFound");
            BEAST_EXPECT(result[jss::status] == "error");
        }

        std::string const txHash {
            "E2FE8D4AF3FCC3944DDF6CD8CDDC5E3F0AD50863EF8919AFEF10CB6408CD4D05"};

        {
            Json::Value const result {env.rpc ("transaction_entry")};
            BEAST_EXPECT(result[jss::ledger_hash].asString().empty());
            BEAST_EXPECT(result[jss::error] == "badSyntax");
            BEAST_EXPECT(result[jss::status] == "error");
        }

        {
            Json::Value const result {env.rpc ("transaction_entry", txHash)};
            BEAST_EXPECT(result[jss::error] == "badSyntax");
            BEAST_EXPECT(result[jss::status] == "error");
        }

        {
            Json::Value const result {env.rpc (
                "transaction_entry", txHash.substr (1), "closed")};
            BEAST_EXPECT(result[jss::error] == "invalidParams");
            BEAST_EXPECT(result[jss::status] == "error");
        }

        {
            Json::Value const result {env.rpc (
                "transaction_entry", txHash + "A", "closed")};
            BEAST_EXPECT(result[jss::error] == "invalidParams");
            BEAST_EXPECT(result[jss::status] == "error");
        }

        {
            Json::Value const result {env.rpc (
                "transaction_entry", txHash, "closer")};
            BEAST_EXPECT(result[jss::error] == "invalidParams");
            BEAST_EXPECT(result[jss::status] == "error");
        }

        {
            Json::Value const result {env.rpc (
                "transaction_entry", txHash, "0")};
            BEAST_EXPECT(result[jss::error] == "invalidParams");
            BEAST_EXPECT(result[jss::status] == "error");
        }

        {
            Json::Value const result {env.rpc (
                "transaction_entry", txHash, "closed", "extra")};
            BEAST_EXPECT(result[jss::error] == "badSyntax");
            BEAST_EXPECT(result[jss::status] == "error");
        }

        {
            Json::Value const result {env.rpc (
                "transaction_entry", txHash, "closed")};
            BEAST_EXPECT(
                ! result[jss::result][jss::ledger_hash].asString().empty());
            BEAST_EXPECT(
                result[jss::result][jss::error] == "transactionNotFound");
            BEAST_EXPECT(result[jss::result][jss::status] == "error");
        }
    }

    void testRequest()
    {
        testcase("Basic request");
        using namespace test::jtx;
        Env env {*this};

        auto check_tx = [this, &env]
            (int index, std::string const txhash, std::string const type = "")
            {
                Json::Value const resIndex {[&env, index, &txhash] ()
                {
                    Json::Value params {Json::objectValue};
                    params[jss::ledger_index] = index;
                    params[jss::tx_hash] = txhash;
                    return env.client()
                        .invoke("transaction_entry", params)[jss::result];
                }()};

                if(! BEAST_EXPECTS(resIndex.isMember(jss::tx_json), txhash))
                    return;

                BEAST_EXPECT(resIndex[jss::tx_json][jss::hash] == txhash);
                if(! type.empty())
                {
                    BEAST_EXPECTS(
                        resIndex[jss::tx_json][jss::TransactionType] == type,
                        txhash + " is " +
                            resIndex[jss::tx_json][jss::TransactionType].asString());
                }

                {
                    Json::Value params {Json::objectValue};
                    params[jss::ledger_hash] = resIndex[jss::ledger_hash];
                    params[jss::tx_hash] = txhash;
                    Json::Value const resHash = env.client()
                        .invoke("transaction_entry", params)[jss::result];
                    BEAST_EXPECT(resHash == resIndex);
                }

                {
                    Json::Value const clIndex {env.rpc (
                        "transaction_entry", txhash, std::to_string (index))};
                    BEAST_EXPECT (clIndex["result"] == resIndex);
                }

                {
                    Json::Value const clHash {env.rpc (
                        "transaction_entry", txhash,
                        resIndex[jss::ledger_hash].asString())};
                    BEAST_EXPECT (clHash["result"] == resIndex);
                }
            };

        Account A1 {"A1"};
        Account A2 {"A2"};

        env.fund(XRP(10000), A1);
        auto fund_1_tx =
            boost::lexical_cast<std::string>(env.tx()->getTransactionID());

        env.fund(XRP(10000), A2);
        auto fund_2_tx =
            boost::lexical_cast<std::string>(env.tx()->getTransactionID());

        env.close();

        check_tx(env.closed()->seq(), fund_1_tx);
        check_tx(env.closed()->seq(), fund_2_tx);

        env.trust(A2["USD"](1000), A1);
        auto trust_tx =
            boost::lexical_cast<std::string>(env.tx()->getTransactionID());

        env(pay(A2, A1, A2["USD"](5)));
        auto pay_tx =
            boost::lexical_cast<std::string>(env.tx()->getTransactionID());
        env.close();

        check_tx(env.closed()->seq(), trust_tx);
        check_tx(env.closed()->seq(), pay_tx, jss::Payment.c_str());

        env(offer(A2, XRP(100), A2["USD"](1)));
        auto offer_tx =
            boost::lexical_cast<std::string>(env.tx()->getTransactionID());

        env.close();

        check_tx(env.closed()->seq(), offer_tx, jss::OfferCreate.c_str());
    }

public:
    void run () override
    {
        testBadInput();
        testRequest();
    }
};

BEAST_DEFINE_TESTSUITE (TransactionEntry, rpc, ripple);

}  
























