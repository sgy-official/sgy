

#include <test/jtx.h>
#include <ripple/app/ledger/BuildLedger.h>
#include <ripple/app/ledger/LedgerMaster.h>
#include <ripple/app/ledger/LedgerReplay.h>

namespace ripple {
namespace test {

struct LedgerReplay_test : public beast::unit_test::suite
{
    void run() override
    {
        testcase("Replay ledger");

        using namespace jtx;

        auto const alice = Account("alice");
        auto const bob = Account("bob");

        Env env(*this);
        env.fund(XRP(100000), alice, bob);
        env.close();

        LedgerMaster& ledgerMaster = env.app().getLedgerMaster();
        auto const lastClosed = ledgerMaster.getClosedLedger();
        auto const lastClosedParent =
            ledgerMaster.getLedgerByHash(lastClosed->info().parentHash);

        auto const replayed = buildLedger(
            LedgerReplay(lastClosedParent,lastClosed),
            tapNONE,
            env.app(),
            env.journal);

        BEAST_EXPECT(replayed->info().hash == lastClosed->info().hash);
    }
};

BEAST_DEFINE_TESTSUITE(LedgerReplay,app,ripple);

} 
} 
























