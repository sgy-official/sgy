
#include <ripple/beast/clock/manual_clock.h>
#include <ripple/beast/unit_test.h>
#include <test/csf.h>
#include <utility>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <string>

namespace ripple {
namespace test {


class DistributedValidators_test : public beast::unit_test::suite
{

    void
    completeTrustCompleteConnectFixedDelay(
            std::size_t numPeers,
            std::chrono::milliseconds delay = std::chrono::milliseconds(200),
            bool printHeaders = false)
    {
        using namespace csf;
        using namespace std::chrono;

        std::string const prefix =
                "DistributedValidators_"
                "completeTrustCompleteConnectFixedDelay";
        std::fstream
                txLog(prefix + "_tx.csv", std::ofstream::app),
                ledgerLog(prefix + "_ledger.csv", std::ofstream::app);

        log << prefix << "(" << numPeers << "," << delay.count() << ")"
            << std::endl;

        BEAST_EXPECT(numPeers >= 1);

        Sim sim;
        PeerGroup peers = sim.createGroup(numPeers);

        peers.trust(peers);

        peers.connect(peers, delay);

        TxCollector txCollector;
        LedgerCollector ledgerCollector;
        auto colls = makeCollectors(txCollector, ledgerCollector);
        sim.collectors.add(colls);

        sim.run(1);

        std::chrono::nanoseconds const simDuration = 10min;
        std::chrono::nanoseconds const quiet = 10s;
        Rate const rate{100, 1000ms};

        HeartbeatTimer heart(sim.scheduler);

        auto peerSelector = makeSelector(peers.begin(),
                                     peers.end(),
                                     std::vector<double>(numPeers, 1.),
                                     sim.rng);
        auto txSubmitter = makeSubmitter(ConstantDistribution{rate.inv()},
                                     sim.scheduler.now() + quiet,
                                     sim.scheduler.now() + simDuration - quiet,
                                     peerSelector,
                                     sim.scheduler,
                                     sim.rng);

        heart.start();
        sim.run(simDuration);


        log << std::right;
        log << "| Peers: "<< std::setw(2) << peers.size();
        log << " | Duration: " << std::setw(6)
            << duration_cast<milliseconds>(simDuration).count() << " ms";
        log << " | Branches: " << std::setw(1) << sim.branches();
        log << " | Synchronized: " << std::setw(1)
            << (sim.synchronized() ? "Y" : "N");
        log << " |" << std::endl;

        txCollector.report(simDuration, log, true);
        ledgerCollector.report(simDuration, log, false);

        std::string const tag = std::to_string(numPeers);
        txCollector.csv(simDuration, txLog, tag, printHeaders);
        ledgerCollector.csv(simDuration, ledgerLog, tag, printHeaders);

        log << std::endl;
    }

    void
    completeTrustScaleFreeConnectFixedDelay(
            std::size_t numPeers,
            std::chrono::milliseconds delay = std::chrono::milliseconds(200),
            bool printHeaders = false)
    {
        using namespace csf;
        using namespace std::chrono;

        std::string const prefix =
                "DistributedValidators__"
                "completeTrustScaleFreeConnectFixedDelay";
        std::fstream
                txLog(prefix + "_tx.csv", std::ofstream::app),
                ledgerLog(prefix + "_ledger.csv", std::ofstream::app);

        log << prefix << "(" << numPeers << "," << delay.count() << ")"
            << std::endl;

        int const numCNLs    = std::max(int(1.00 * numPeers), 1);
        int const minCNLSize = std::max(int(0.25 * numCNLs),  1);
        int const maxCNLSize = std::max(int(0.50 * numCNLs),  1);
        BEAST_EXPECT(numPeers >= 1);
        BEAST_EXPECT(numCNLs >= 1);
        BEAST_EXPECT(1 <= minCNLSize
                && minCNLSize <= maxCNLSize
                && maxCNLSize <= numPeers);

        Sim sim;
        PeerGroup peers = sim.createGroup(numPeers);

        peers.trust(peers);

        std::vector<double> const ranks =
                sample(peers.size(), PowerLawDistribution{1, 3}, sim.rng);
        randomRankedConnect(peers, ranks, numCNLs,
                std::uniform_int_distribution<>{minCNLSize, maxCNLSize},
                sim.rng, delay);

        TxCollector txCollector;
        LedgerCollector ledgerCollector;
        auto colls = makeCollectors(txCollector, ledgerCollector);
        sim.collectors.add(colls);

        sim.run(1);

        std::chrono::nanoseconds simDuration = 10min;
        std::chrono::nanoseconds quiet = 10s;
        Rate rate{100, 1000ms};

        HeartbeatTimer heart(sim.scheduler);

        auto peerSelector = makeSelector(peers.begin(),
                                     peers.end(),
                                     std::vector<double>(numPeers, 1.),
                                     sim.rng);
        auto txSubmitter = makeSubmitter(ConstantDistribution{rate.inv()},
                                     sim.scheduler.now() + quiet,
                                     sim.scheduler.now() + simDuration - quiet,
                                     peerSelector,
                                     sim.scheduler,
                                     sim.rng);

        heart.start();
        sim.run(simDuration);


        log << std::right;
        log << "| Peers: "<< std::setw(2) << peers.size();
        log << " | Duration: " << std::setw(6)
            << duration_cast<milliseconds>(simDuration).count() << " ms";
        log << " | Branches: " << std::setw(1) << sim.branches();
        log << " | Synchronized: " << std::setw(1)
            << (sim.synchronized() ? "Y" : "N");
        log << " |" << std::endl;

        txCollector.report(simDuration, log, true);
        ledgerCollector.report(simDuration, log, false);

        std::string const tag = std::to_string(numPeers);
        txCollector.csv(simDuration, txLog, tag, printHeaders);
        ledgerCollector.csv(simDuration, ledgerLog, tag, printHeaders);

        log << std::endl;
    }

    void
    run() override
    {
        std::string const defaultArgs = "5 200";
        std::string const args = arg().empty() ? defaultArgs : arg();
        std::stringstream argStream(args);

        int maxNumValidators = 0;
        int delayCount(200);
        argStream >> maxNumValidators;
        argStream >> delayCount;

        std::chrono::milliseconds const delay(delayCount);

        log << "DistributedValidators: 1 to " << maxNumValidators << " Peers"
            << std::endl;

        
        completeTrustCompleteConnectFixedDelay(1, delay, true);
        for(int i = 2; i <= maxNumValidators; i++)
        {
            completeTrustCompleteConnectFixedDelay(i, delay);
        }

        
        completeTrustScaleFreeConnectFixedDelay(1, delay, true);
        for(int i = 2; i <= maxNumValidators; i++)
        {
            completeTrustScaleFreeConnectFixedDelay(i, delay);
        }
    }
};

BEAST_DEFINE_TESTSUITE_MANUAL_PRIO(DistributedValidators, consensus, ripple, 2);

}  
}  
























