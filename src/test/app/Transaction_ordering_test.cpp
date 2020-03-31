

#include <ripple/core/JobQueue.h>
#include <ripple/protocol/ErrorCodes.h>
#include <test/jtx.h>

namespace ripple {
namespace test {

struct Transaction_ordering_test : public beast::unit_test::suite
{
    void testCorrectOrder()
    {
        using namespace jtx;

        Env env(*this);
        auto const alice = Account("alice");
        env.fund(XRP(1000), noripple(alice));

        auto const aliceSequence = env.seq(alice);

        auto const tx1 = env.jt(noop(alice), seq(aliceSequence));
        auto const tx2 = env.jt(noop(alice), seq(aliceSequence + 1),
            json(R"({"LastLedgerSequence":7})"));

        env(tx1);
        env.close();
        BEAST_EXPECT(env.seq(alice) == aliceSequence + 1);
        env(tx2);
        env.close();
        BEAST_EXPECT(env.seq(alice) == aliceSequence + 2);

        env.close();

        {
            auto const result = env.rpc("tx", to_string(tx1.stx->getTransactionID()));
            BEAST_EXPECT(result["result"]["meta"]["TransactionResult"] == "tesSUCCESS");
        }
        {
            auto const result = env.rpc("tx", to_string(tx2.stx->getTransactionID()));
            BEAST_EXPECT(result["result"]["meta"]["TransactionResult"] == "tesSUCCESS");
        }
    }

    void testIncorrectOrder()
    {
        using namespace jtx;

        Env env(*this);
        env.app().getJobQueue().setThreadCount(0, false);
        auto const alice = Account("alice");
        env.fund(XRP(1000), noripple(alice));

        auto const aliceSequence = env.seq(alice);

        auto const tx1 = env.jt(noop(alice), seq(aliceSequence));
        auto const tx2 = env.jt(noop(alice), seq(aliceSequence + 1),
            json(R"({"LastLedgerSequence":7})"));

        env(tx2, ter(terPRE_SEQ));
        BEAST_EXPECT(env.seq(alice) == aliceSequence);
        env(tx1);
        env.app().getJobQueue().rendezvous();
        BEAST_EXPECT(env.seq(alice) == aliceSequence + 2);

        env.close();

        {
            auto const result = env.rpc("tx", to_string(tx1.stx->getTransactionID()));
            BEAST_EXPECT(result["result"]["meta"]["TransactionResult"] == "tesSUCCESS");
        }
        {
            auto const result = env.rpc("tx", to_string(tx2.stx->getTransactionID()));
            BEAST_EXPECT(result["result"]["meta"]["TransactionResult"] == "tesSUCCESS");
        }
    }

    void testIncorrectOrderMultipleIntermediaries()
    {
        using namespace jtx;

        Env env(*this);
        env.app().getJobQueue().setThreadCount(0, false);
        auto const alice = Account("alice");
        env.fund(XRP(1000), noripple(alice));

        auto const aliceSequence = env.seq(alice);

        std::vector<JTx> tx;
        for (auto i = 0; i < 5; ++i)
        {
            tx.emplace_back(
                env.jt(noop(alice), seq(aliceSequence + i),
                    json(R"({"LastLedgerSequence":7})"))
            );
        }

        for (auto i = 1; i < 5; ++i)
        {
            env(tx[i], ter(terPRE_SEQ));
            BEAST_EXPECT(env.seq(alice) == aliceSequence);
        }

        env(tx[0]);
        env.app().getJobQueue().rendezvous();
        BEAST_EXPECT(env.seq(alice) == aliceSequence + 5);

        env.close();

        for (auto i = 0; i < 5; ++i)
        {
            auto const result = env.rpc("tx", to_string(tx[i].stx->getTransactionID()));
            BEAST_EXPECT(result["result"]["meta"]["TransactionResult"] == "tesSUCCESS");
        }
    }

    void run() override
    {
        testCorrectOrder();
        testIncorrectOrder();
        testIncorrectOrderMultipleIntermediaries();
    }
};

BEAST_DEFINE_TESTSUITE(Transaction_ordering,app,ripple);

} 
} 
























