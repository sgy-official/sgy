

#include <ripple/core/JobQueue.h>
#include <ripple/protocol/jss.h>
#include <test/jtx.h>
#include <test/jtx/WSClient.h>
#include <ripple/beast/unit_test.h>

namespace ripple {
namespace test {

class RobustTransaction_test : public beast::unit_test::suite
{
public:
    void
    testSequenceRealignment()
    {
        using namespace std::chrono_literals;
        using namespace jtx;
        Env env(*this);
        env.fund(XRP(10000), "alice", "bob");
        env.close();
        auto wsc = makeWSClient(env.app().config());

        {
            Json::Value jv;
            jv[jss::streams] = Json::arrayValue;
            jv[jss::streams].append("transactions");
            jv = wsc->invoke("subscribe", jv);
            BEAST_EXPECT(jv[jss::status] == "success");
            if (wsc->version() == 2)
            {
                BEAST_EXPECT(jv.isMember(jss::jsonrpc) && jv[jss::jsonrpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::ripplerpc) && jv[jss::ripplerpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::id) && jv[jss::id] == 5);
            }
        }

        {
            Json::Value payment;
            payment[jss::secret] = toBase58(generateSeed("alice"));
            payment[jss::tx_json] = pay("alice", "bob", XRP(1));
            payment[jss::tx_json][sfLastLedgerSequence.fieldName] = 1;
            auto jv = wsc->invoke("submit", payment);
            if (wsc->version() == 2)
            {
                BEAST_EXPECT(jv.isMember(jss::jsonrpc) && jv[jss::jsonrpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::ripplerpc) && jv[jss::ripplerpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::id) && jv[jss::id] == 5);
            }
            BEAST_EXPECT(jv[jss::result][jss::engine_result] ==
                "tefMAX_LEDGER");

            payment[jss::tx_json] = pay("alice", "bob", XRP(1));
            payment[jss::tx_json][sfSequence.fieldName] =
                env.seq("alice") - 1;
            jv = wsc->invoke("submit", payment);
            if (wsc->version() == 2)
            {
                BEAST_EXPECT(jv.isMember(jss::jsonrpc) && jv[jss::jsonrpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::ripplerpc) && jv[jss::ripplerpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::id) && jv[jss::id] == 5);
            }
            BEAST_EXPECT(jv[jss::result][jss::engine_result] ==
                "tefPAST_SEQ");

            payment[jss::tx_json][sfSequence.fieldName] =
                env.seq("alice") + 1;
            jv = wsc->invoke("submit", payment);
            if (wsc->version() == 2)
            {
                BEAST_EXPECT(jv.isMember(jss::jsonrpc) && jv[jss::jsonrpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::ripplerpc) && jv[jss::ripplerpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::id) && jv[jss::id] == 5);
            }
            BEAST_EXPECT(jv[jss::result][jss::engine_result] ==
                "terPRE_SEQ");

            payment[jss::tx_json][sfSequence.fieldName] =
                env.seq("alice");
            jv = wsc->invoke("submit", payment);
            if (wsc->version() == 2)
            {
                BEAST_EXPECT(jv.isMember(jss::jsonrpc) && jv[jss::jsonrpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::ripplerpc) && jv[jss::ripplerpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::id) && jv[jss::id] == 5);
            }
            BEAST_EXPECT(jv[jss::result][jss::engine_result] ==
                "tesSUCCESS");

            env.app().getJobQueue().rendezvous();

            jv = wsc->invoke("ledger_accept");
            if (wsc->version() == 2)
            {
                BEAST_EXPECT(jv.isMember(jss::jsonrpc) && jv[jss::jsonrpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::ripplerpc) && jv[jss::ripplerpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::id) && jv[jss::id] == 5);
            }
            BEAST_EXPECT(jv[jss::result].isMember(
                jss::ledger_current_index));
        }

        {
            BEAST_EXPECT(wsc->findMsg(5s,
                [&](auto const& jv)
                {
                    auto const& ff = jv[jss::meta]["AffectedNodes"]
                        [1u]["ModifiedNode"]["FinalFields"];
                    return ff[jss::Account] == Account("bob").human() &&
                        ff["Balance"] == "10001000000";
                }));

            BEAST_EXPECT(wsc->findMsg(5s,
                [&](auto const& jv)
                {
                    auto const& ff = jv[jss::meta]["AffectedNodes"]
                        [1u]["ModifiedNode"]["FinalFields"];
                    return ff[jss::Account] == Account("bob").human() &&
                        ff["Balance"] == "10002000000";
                }));
        }

        {
            Json::Value jv;
            jv[jss::streams] = Json::arrayValue;
            jv[jss::streams].append("transactions");
            jv = wsc->invoke("unsubscribe", jv);
            if (wsc->version() == 2)
            {
                BEAST_EXPECT(jv.isMember(jss::jsonrpc) && jv[jss::jsonrpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::ripplerpc) && jv[jss::ripplerpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::id) && jv[jss::id] == 5);
            }
            BEAST_EXPECT(jv[jss::status] == "success");
        }
    }

    
    void
    testReconnect()
    {
        using namespace jtx;
        Env env(*this);
        env.fund(XRP(10000), "alice", "bob");
        env.close();
        auto wsc = makeWSClient(env.app().config());

        {
            Json::Value jv;
            jv[jss::secret] = toBase58(generateSeed("alice"));
            jv[jss::tx_json] = pay("alice", "bob", XRP(1));
            jv = wsc->invoke("submit", jv);
            if (wsc->version() == 2)
            {
                BEAST_EXPECT(jv.isMember(jss::jsonrpc) && jv[jss::jsonrpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::ripplerpc) && jv[jss::ripplerpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::id) && jv[jss::id] == 5);
            }
            BEAST_EXPECT(jv[jss::result][jss::engine_result] ==
                "tesSUCCESS");

            wsc.reset();

            env.close();
        }

        {
            Json::Value jv;
            jv[jss::account] = Account("bob").human();
            jv[jss::ledger_index_min] = -1;
            jv[jss::ledger_index_max] = -1;
            wsc = makeWSClient(env.app().config());
            jv = wsc->invoke("account_tx", jv);
            if (wsc->version() == 2)
            {
                BEAST_EXPECT(jv.isMember(jss::jsonrpc) && jv[jss::jsonrpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::ripplerpc) && jv[jss::ripplerpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::id) && jv[jss::id] == 5);
            }

            auto ff = jv[jss::result][jss::transactions][0u][jss::meta]
                ["AffectedNodes"][1u]["ModifiedNode"]["FinalFields"];
            BEAST_EXPECT(ff[jss::Account] ==
                Account("bob").human());
            BEAST_EXPECT(ff["Balance"] == "10001000000");
        }
    }

    void
    testReconnectAfterWait()
    {
        using namespace std::chrono_literals;
        using namespace jtx;
        Env env(*this);
        env.fund(XRP(10000), "alice", "bob");
        env.close();
        auto wsc = makeWSClient(env.app().config());

        {
            Json::Value jv;
            jv[jss::secret] = toBase58(generateSeed("alice"));
            jv[jss::tx_json] = pay("alice", "bob", XRP(1));
            jv = wsc->invoke("submit", jv);
            if (wsc->version() == 2)
            {
                BEAST_EXPECT(jv.isMember(jss::jsonrpc) && jv[jss::jsonrpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::ripplerpc) && jv[jss::ripplerpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::id) && jv[jss::id] == 5);
            }
            BEAST_EXPECT(jv[jss::result][jss::engine_result] ==
                "tesSUCCESS");

            jv = wsc->invoke("ledger_accept");
            if (wsc->version() == 2)
            {
                BEAST_EXPECT(jv.isMember(jss::jsonrpc) && jv[jss::jsonrpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::ripplerpc) && jv[jss::ripplerpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::id) && jv[jss::id] == 5);
            }
            BEAST_EXPECT(jv[jss::result].isMember(
                jss::ledger_current_index));

            env.app().getJobQueue().rendezvous();
        }

        {
            {
                Json::Value jv;
                jv[jss::streams] = Json::arrayValue;
                jv[jss::streams].append("ledger");
                jv = wsc->invoke("subscribe", jv);
                if (wsc->version() == 2)
                {
                    BEAST_EXPECT(jv.isMember(jss::jsonrpc) && jv[jss::jsonrpc] == "2.0");
                    BEAST_EXPECT(jv.isMember(jss::ripplerpc) && jv[jss::ripplerpc] == "2.0");
                    BEAST_EXPECT(jv.isMember(jss::id) && jv[jss::id] == 5);
                }
                BEAST_EXPECT(jv[jss::status] == "success");
            }

            for(auto i = 0; i < 8; ++i)
            {
                auto jv = wsc->invoke("ledger_accept");
                if (wsc->version() == 2)
                {
                    BEAST_EXPECT(jv.isMember(jss::jsonrpc) && jv[jss::jsonrpc] == "2.0");
                    BEAST_EXPECT(jv.isMember(jss::ripplerpc) && jv[jss::ripplerpc] == "2.0");
                    BEAST_EXPECT(jv.isMember(jss::id) && jv[jss::id] == 5);
                }
                BEAST_EXPECT(jv[jss::result].
                    isMember(jss::ledger_current_index));

                env.app().getJobQueue().rendezvous();

                BEAST_EXPECT(wsc->findMsg(5s,
                    [&](auto const& jv)
                    {
                        return jv[jss::type] == "ledgerClosed";
                    }));
            }

            {
                Json::Value jv;
                jv[jss::streams] = Json::arrayValue;
                jv[jss::streams].append("ledger");
                jv = wsc->invoke("unsubscribe", jv);
                if (wsc->version() == 2)
                {
                    BEAST_EXPECT(jv.isMember(jss::jsonrpc) && jv[jss::jsonrpc] == "2.0");
                    BEAST_EXPECT(jv.isMember(jss::ripplerpc) && jv[jss::ripplerpc] == "2.0");
                    BEAST_EXPECT(jv.isMember(jss::id) && jv[jss::id] == 5);
                }
                BEAST_EXPECT(jv[jss::status] == "success");
            }
        }

        {
            wsc = makeWSClient(env.app().config());
            {
                Json::Value jv;
                jv[jss::streams] = Json::arrayValue;
                jv[jss::streams].append("ledger");
                jv = wsc->invoke("subscribe", jv);
                if (wsc->version() == 2)
                {
                    BEAST_EXPECT(jv.isMember(jss::jsonrpc) && jv[jss::jsonrpc] == "2.0");
                    BEAST_EXPECT(jv.isMember(jss::ripplerpc) && jv[jss::ripplerpc] == "2.0");
                    BEAST_EXPECT(jv.isMember(jss::id) && jv[jss::id] == 5);
                }
                BEAST_EXPECT(jv[jss::status] == "success");
            }

            for (auto i = 0; i < 2; ++i)
            {
                auto jv = wsc->invoke("ledger_accept");
                if (wsc->version() == 2)
                {
                    BEAST_EXPECT(jv.isMember(jss::jsonrpc) && jv[jss::jsonrpc] == "2.0");
                    BEAST_EXPECT(jv.isMember(jss::ripplerpc) && jv[jss::ripplerpc] == "2.0");
                    BEAST_EXPECT(jv.isMember(jss::id) && jv[jss::id] == 5);
                }
                BEAST_EXPECT(jv[jss::result].
                    isMember(jss::ledger_current_index));

                env.app().getJobQueue().rendezvous();

                BEAST_EXPECT(wsc->findMsg(5s,
                    [&](auto const& jv)
                    {
                        return jv[jss::type] == "ledgerClosed";
                    }));
            }

            {
                Json::Value jv;
                jv[jss::streams] = Json::arrayValue;
                jv[jss::streams].append("ledger");
                jv = wsc->invoke("unsubscribe", jv);
                if (wsc->version() == 2)
                {
                    BEAST_EXPECT(jv.isMember(jss::jsonrpc) && jv[jss::jsonrpc] == "2.0");
                    BEAST_EXPECT(jv.isMember(jss::ripplerpc) && jv[jss::ripplerpc] == "2.0");
                    BEAST_EXPECT(jv.isMember(jss::id) && jv[jss::id] == 5);
                }
                BEAST_EXPECT(jv[jss::status] == "success");
            }
        }

        {
            Json::Value jv;
            jv[jss::account] = Account("bob").human();
            jv[jss::ledger_index_min] = -1;
            jv[jss::ledger_index_max] = -1;
            wsc = makeWSClient(env.app().config());
            jv = wsc->invoke("account_tx", jv);
            if (wsc->version() == 2)
            {
                BEAST_EXPECT(jv.isMember(jss::jsonrpc) && jv[jss::jsonrpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::ripplerpc) && jv[jss::ripplerpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::id) && jv[jss::id] == 5);
            }

            auto ff = jv[jss::result][jss::transactions][0u][jss::meta]
                ["AffectedNodes"][1u]["ModifiedNode"]["FinalFields"];
            BEAST_EXPECT(ff[jss::Account] ==
                Account("bob").human());
            BEAST_EXPECT(ff["Balance"] == "10001000000");
        }
    }

    void
    testAccountsProposed()
    {
        using namespace std::chrono_literals;
        using namespace jtx;
        Env env(*this);
        env.fund(XRP(10000), "alice");
        env.close();
        auto wsc = makeWSClient(env.app().config());

        {
            Json::Value jv;
            jv[jss::accounts_proposed] = Json::arrayValue;
            jv[jss::accounts_proposed].append(
                Account("alice").human());
            jv = wsc->invoke("subscribe", jv);
            if (wsc->version() == 2)
            {
                BEAST_EXPECT(jv.isMember(jss::jsonrpc) && jv[jss::jsonrpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::ripplerpc) && jv[jss::ripplerpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::id) && jv[jss::id] == 5);
            }
            BEAST_EXPECT(jv[jss::status] == "success");
        }

        {
            Json::Value jv;
            jv[jss::secret] = toBase58(generateSeed("alice"));
            jv[jss::tx_json] = fset("alice", 0);
            jv[jss::tx_json][jss::Fee] = 10;
            jv = wsc->invoke("submit", jv);
            if (wsc->version() == 2)
            {
                BEAST_EXPECT(jv.isMember(jss::jsonrpc) && jv[jss::jsonrpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::ripplerpc) && jv[jss::ripplerpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::id) && jv[jss::id] == 5);
            }
            BEAST_EXPECT(jv[jss::result][jss::engine_result] ==
                "tesSUCCESS");
        }

        {
            BEAST_EXPECT(wsc->findMsg(5s,
                [&](auto const& jv)
                {
                    return jv[jss::transaction][jss::TransactionType] ==
                        jss::AccountSet;
                }));
        }

        {
            Json::Value jv;
            jv[jss::accounts_proposed] = Json::arrayValue;
            jv[jss::accounts_proposed].append(
                Account("alice").human());
            jv = wsc->invoke("unsubscribe", jv);
            if (wsc->version() == 2)
            {
                BEAST_EXPECT(jv.isMember(jss::jsonrpc) && jv[jss::jsonrpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::ripplerpc) && jv[jss::ripplerpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::id) && jv[jss::id] == 5);
            }
            BEAST_EXPECT(jv[jss::status] == "success");
        }
    }

    void
    run() override
    {
        testSequenceRealignment();
        testReconnect();
        testReconnectAfterWait();
        testAccountsProposed();
    }
};

BEAST_DEFINE_TESTSUITE(RobustTransaction,app,ripple);

} 
} 
























