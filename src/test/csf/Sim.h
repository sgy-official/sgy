

#ifndef RIPPLE_TEST_CSF_SIM_H_INCLUDED
#define RIPPLE_TEST_CSF_SIM_H_INCLUDED

#include <test/csf/Digraph.h>
#include <test/csf/SimTime.h>
#include <test/csf/BasicNetwork.h>
#include <test/csf/Scheduler.h>
#include <test/csf/Peer.h>
#include <test/csf/PeerGroup.h>
#include <test/csf/TrustGraph.h>
#include <test/csf/CollectorRef.h>

#include <iostream>
#include <deque>
#include <random>

namespace ripple {
namespace test {
namespace csf {


class BasicSink : public beast::Journal::Sink
{
    Scheduler::clock_type const & clock_;
public:
    BasicSink (Scheduler::clock_type const & clock)
        : Sink (beast::severities::kDisabled, false)
        , clock_{clock}
    {
    }

    void
    write (beast::severities::Severity level,
        std::string const& text) override
    {
        if (level < threshold())
            return;

        std::cout << clock_.now().time_since_epoch().count() << " " << text
                  << std::endl;
    }
};

class Sim
{
    std::deque<Peer> peers;
    PeerGroup allPeers;

public:
    std::mt19937_64 rng;
    Scheduler scheduler;
    BasicSink sink;
    beast::Journal j;
    LedgerOracle oracle;
    BasicNetwork<Peer*> net;
    TrustGraph<Peer*> trustGraph;
    CollectorRefs collectors;

    
    Sim() : sink{scheduler.clock()}, j{sink}, net{scheduler}
    {
    }

    
    PeerGroup
    createGroup(std::size_t numPeers)
    {
        std::vector<Peer*> newPeers;
        newPeers.reserve(numPeers);
        for (std::size_t i = 0; i < numPeers; ++i)
        {
            peers.emplace_back(
                PeerID{static_cast<std::uint32_t>(peers.size())},
                scheduler,
                oracle,
                net,
                trustGraph,
                collectors,
                j);
            newPeers.emplace_back(&peers.back());
        }
        PeerGroup res{newPeers};
        allPeers = allPeers + res;
        return res;
    }

    std::size_t
    size() const
    {
        return peers.size();
    }

    
    void
    run(int ledgers);

    
    void
    run(SimDuration const& dur);

    
    bool
    synchronized(PeerGroup const& g) const;

    
    bool
    synchronized() const;

    
    std::size_t
    branches(PeerGroup const& g) const;

    
    std::size_t
    branches() const;

};

}  
}  
}  

#endif








