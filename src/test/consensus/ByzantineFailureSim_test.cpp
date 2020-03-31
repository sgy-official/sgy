
#include <ripple/beast/clock/manual_clock.h>
#include <ripple/beast/unit_test.h>
#include <test/csf.h>
#include <utility>

namespace ripple {
namespace test {

class ByzantineFailureSim_test : public beast::unit_test::suite
{
    void
    run() override
    {
        using namespace csf;
        using namespace std::chrono;


        Sim sim;
        ConsensusParms const parms{};

        SimDuration const delay =
            date::round<milliseconds>(0.2 * parms.ledgerGRANULARITY);
        PeerGroup a = sim.createGroup(1);
        PeerGroup b = sim.createGroup(1);
        PeerGroup c = sim.createGroup(1);
        PeerGroup d = sim.createGroup(1);
        PeerGroup e = sim.createGroup(1);
        PeerGroup f = sim.createGroup(1);
        PeerGroup g = sim.createGroup(1);

        a.trustAndConnect(a + b + c + g, delay);
        b.trustAndConnect(b + a + c + d + e, delay);
        c.trustAndConnect(c + a + b + d + e, delay);
        d.trustAndConnect(d + b + c + e + f, delay);
        e.trustAndConnect(e + b + c + d + f, delay);
        f.trustAndConnect(f + d + e + g, delay);
        g.trustAndConnect(g + a + f, delay);

        PeerGroup network = a + b + c + d + e + f + g;

        StreamCollector sc{std::cout};

        sim.collectors.add(sc);

        for (TrustGraph<Peer*>::ForkInfo const& fi :
             sim.trustGraph.forkablePairs(0.8))
        {
            std::cout << "Can fork " << PeerGroup{fi.unlA} << " "
                      << " " << PeerGroup{fi.unlB}
                      << " overlap " << fi.overlap << " required "
                      << fi.required << "\n";
        };

        sim.run(1);

        PeerGroup byzantineNodes = a + b + c + g;
        for (Peer * peer : network)
        {
            peer->submit(Tx(0));
            if (byzantineNodes.contains(peer))
            {
                peer->txInjections.emplace(
                    peer->lastClosedLedger.seq(), Tx{42});
            }
        }
        sim.run(4);
        std::cout << "Branches: " << sim.branches() << "\n";
        std::cout << "Fully synchronized: " << std::boolalpha
                  << sim.synchronized() << "\n";
        pass();
    }
};

BEAST_DEFINE_TESTSUITE_MANUAL(ByzantineFailureSim, consensus, ripple);

}  
}  
























