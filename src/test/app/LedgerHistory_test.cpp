

#include <ripple/app/ledger/LedgerHistory.h>
#include <ripple/app/ledger/LedgerMaster.h>
#include <ripple/app/tx/apply.h>
#include <ripple/beast/insight/NullCollector.h>
#include <ripple/beast/unit_test.h>
#include <ripple/ledger/OpenView.h>
#include <chrono>
#include <memory>
#include <sstream>
#include <test/jtx.h>

namespace ripple {
namespace test {

class LedgerHistory_test : public beast::unit_test::suite
{
public:
    
    class CheckMessageLogs : public Logs
    {
        std::string msg_;
        bool& found_;

        class CheckMessageSink : public beast::Journal::Sink
        {
            CheckMessageLogs& owner_;

        public:
            CheckMessageSink(
                beast::severities::Severity threshold,
                CheckMessageLogs& owner)
                : beast::Journal::Sink(threshold, false), owner_(owner)
            {
            }

            void
            write(beast::severities::Severity level, std::string const& text)
                override
            {
                if (text.find(owner_.msg_) != std::string::npos)
                    owner_.found_ = true;
            }
        };

    public:
        
        CheckMessageLogs(std::string msg, bool& found)
            : Logs{beast::severities::kDebug}
            , msg_{std::move(msg)}
            , found_{found}
        {
        }

        std::unique_ptr<beast::Journal::Sink>
        makeSink(
            std::string const& partition,
            beast::severities::Severity threshold) override
        {
            return std::make_unique<CheckMessageSink>(threshold, *this);
        }
    };

    
    static std::shared_ptr<Ledger>
    makeLedger(
        std::shared_ptr<Ledger const> const& prev,
        jtx::Env& env,
        LedgerHistory& lh,
        NetClock::duration closeOffset,
        std::shared_ptr<STTx const> stx = {})
    {
        if (!prev)
        {
            assert(!stx);
            return std::make_shared<Ledger>(
                create_genesis,
                env.app().config(),
                std::vector<uint256>{},
                env.app().family());
        }
        auto res = std::make_shared<Ledger>(
            *prev, prev->info().closeTime + closeOffset);

        if (stx)
        {
            OpenView accum(&*res);
            applyTransaction(
                env.app(), accum, *stx, false, tapNONE, env.journal);
            accum.apply(*res);
        }
        res->updateSkipList();

        {
            res->stateMap().flushDirty(hotACCOUNT_NODE, res->info().seq);
            res->txMap().flushDirty(hotTRANSACTION_NODE, res->info().seq);
        }
        res->unshare();

        res->setAccepted(
            res->info().closeTime,
            res->info().closeTimeResolution,
            true ,
            env.app().config());
        lh.insert(res, false);
        return res;
    }

    void
    testHandleMismatch()
    {
        testcase("LedgerHistory mismatch");
        using namespace jtx;
        using namespace std::chrono;

        {
            bool found = false;
            Env env{*this,
                    envconfig(),
                    std::make_unique<CheckMessageLogs>("MISMATCH ", found)};
            LedgerHistory lh{beast::insight::NullCollector::New(), env.app()};
            auto const genesis = makeLedger({}, env, lh, 0s);
            uint256 const dummyTxHash{1};
            lh.builtLedger(genesis, dummyTxHash, {});
            lh.validatedLedger(genesis, dummyTxHash);

            BEAST_EXPECT(!found);
        }

        {
            bool found = false;
            Env env{*this,
                    envconfig(),
                    std::make_unique<CheckMessageLogs>(
                        "MISMATCH on close time", found)};
            LedgerHistory lh{beast::insight::NullCollector::New(), env.app()};
            auto const genesis = makeLedger({}, env, lh, 0s);
            auto const ledgerA = makeLedger(genesis, env, lh, 4s);
            auto const ledgerB = makeLedger(genesis, env, lh, 40s);

            uint256 const dummyTxHash{1};
            lh.builtLedger(ledgerA, dummyTxHash, {});
            lh.validatedLedger(ledgerB, dummyTxHash);

            BEAST_EXPECT(found);
        }

        {
            bool found = false;
            Env env{*this,
                    envconfig(),
                    std::make_unique<CheckMessageLogs>(
                        "MISMATCH on prior ledger", found)};
            LedgerHistory lh{beast::insight::NullCollector::New(), env.app()};
            auto const genesis = makeLedger({}, env, lh, 0s);
            auto const ledgerA = makeLedger(genesis, env, lh, 4s);
            auto const ledgerB = makeLedger(genesis, env, lh, 40s);
            auto const ledgerAC = makeLedger(ledgerA, env, lh, 4s);
            auto const ledgerBD = makeLedger(ledgerB, env, lh, 4s);

            uint256 const dummyTxHash{1};
            lh.builtLedger(ledgerAC, dummyTxHash, {});
            lh.validatedLedger(ledgerBD, dummyTxHash);

            BEAST_EXPECT(found);
        }

        for (bool const txBug : {true, false})
        {
            std::string const msg = txBug
                ? "MISMATCH with same consensus transaction set"
                : "MISMATCH on consensus transaction set";
            bool found = false;
            Env env{*this,
                    envconfig(),
                    std::make_unique<CheckMessageLogs>(msg, found)};
            LedgerHistory lh{beast::insight::NullCollector::New(), env.app()};

            Account alice{"A1"};
            Account bob{"A2"};
            env.fund(XRP(1000), alice, bob);
            env.close();

            auto const ledgerBase =
                env.app().getLedgerMaster().getClosedLedger();

            JTx txAlice = env.jt(noop(alice));
            auto const ledgerA =
                makeLedger(ledgerBase, env, lh, 4s, txAlice.stx);

            JTx txBob = env.jt(noop(bob));
            auto const ledgerB = makeLedger(ledgerBase, env, lh, 4s, txBob.stx);

            lh.builtLedger(ledgerA, txAlice.stx->getTransactionID(), {});
            lh.validatedLedger(
                ledgerB,
                txBug ? txAlice.stx->getTransactionID()
                      : txBob.stx->getTransactionID());

            BEAST_EXPECT(found);
        }
    }

    void
    run() override
    {
        testHandleMismatch();
    }
};

BEAST_DEFINE_TESTSUITE(LedgerHistory, app, ripple);

}  
}  
























