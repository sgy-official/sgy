
#include <ripple/beast/clock/manual_clock.h>
#include <ripple/beast/unit_test.h>
#include <test/csf.h>
#include <test/csf/random.h>
#include <utility>

namespace ripple {
namespace test {

class ScaleFreeSim_test : public beast::unit_test::suite
{
    void
    run() override
    {
        using namespace std::chrono;
        using namespace csf;



        int const N = 100;  

        int const numUNLs = 15;  
        int const minUNLSize = N / 4, maxUNLSize = N / 2;

        ConsensusParms const parms{};
        Sim sim;
        PeerGroup network = sim.createGroup(N);

        std::vector<double> const ranks =
            sample(network.size(), PowerLawDistribution{1, 3}, sim.rng);

        randomRankedTrust(network, ranks, numUNLs,
            std::uniform_int_distribution<>{minUNLSize, maxUNLSize},
            sim.rng);

        network.connectFromTrust(
            date::round<milliseconds>(0.2 * parms.ledgerGRANULARITY));

        TxCollector txCollector;
        LedgerCollector ledgerCollector;
        auto colls = makeCollectors(txCollector, ledgerCollector);
        sim.collectors.add(colls);

        sim.run(1);

        HeartbeatTimer heart(sim.scheduler, seconds(10s));

        std::chrono::nanoseconds const simDuration = 10min;
        std::chrono::nanoseconds const quiet = 10s;
        Rate const rate{100, 1000ms};

        auto peerSelector = makeSelector(network.begin(),
                                     network.end(),
                                     ranks,
                                     sim.rng);
        auto txSubmitter = makeSubmitter(ConstantDistribution{rate.inv()},
                          sim.scheduler.now() + quiet,
                          sim.scheduler.now() + (simDuration - quiet),
                          peerSelector,
                          sim.scheduler,
                          sim.rng);

        heart.start();
        sim.run(simDuration);

        BEAST_EXPECT(sim.branches() == 1);
        BEAST_EXPECT(sim.synchronized());


        log << "Peers: " << network.size() << std::endl;
        log << "Simulated Duration: "
            << duration_cast<milliseconds>(simDuration).count()
            << " ms" << std::endl;
        log << "Branches: " << sim.branches() << std::endl;
        log << "Synchronized: " << (sim.synchronized() ? "Y" : "N")
            << std::endl;
        log << std::endl;

        txCollector.report(simDuration, log);
        ledgerCollector.report(simDuration, log);
    }
};

BEAST_DEFINE_TESTSUITE_MANUAL_PRIO(ScaleFreeSim, consensus, ripple, 80);

}  
}  
























