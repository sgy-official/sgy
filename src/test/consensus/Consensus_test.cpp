
#include <ripple/beast/clock/manual_clock.h>
#include <ripple/beast/unit_test.h>
#include <ripple/consensus/Consensus.h>
#include <ripple/consensus/ConsensusProposal.h>
#include <test/csf.h>
#include <test/unit_test/SuiteJournal.h>
#include <utility>

namespace ripple {
namespace test {

class Consensus_test : public beast::unit_test::suite
{
    SuiteJournal journal_;

public:
    Consensus_test ()
    : journal_ ("Consensus_test", *this)
    { }

    void
    testShouldCloseLedger()
    {
        using namespace std::chrono_literals;

        ConsensusParms const p{};

        BEAST_EXPECT(
            shouldCloseLedger(true, 10, 10, 10, -10s, 10s, 1s, 1s, p, journal_));
        BEAST_EXPECT(
            shouldCloseLedger(true, 10, 10, 10, 100h, 10s, 1s, 1s, p, journal_));
        BEAST_EXPECT(
            shouldCloseLedger(true, 10, 10, 10, 10s, 100h, 1s, 1s, p, journal_));

        BEAST_EXPECT(
            shouldCloseLedger(true, 10, 3, 5, 10s, 10s, 10s, 10s, p, journal_));

        BEAST_EXPECT(
            !shouldCloseLedger(false, 10, 0, 0, 1s, 1s, 1s, 10s, p, journal_));
        BEAST_EXPECT(
            shouldCloseLedger(false, 10, 0, 0, 1s, 10s, 1s, 10s, p, journal_));

        BEAST_EXPECT(
            !shouldCloseLedger(true, 10, 0, 0, 10s, 10s, 1s, 10s, p, journal_));

        BEAST_EXPECT(
            !shouldCloseLedger(true, 10, 0, 0, 10s, 10s, 3s, 10s, p, journal_));

        BEAST_EXPECT(
            shouldCloseLedger(true, 10, 0, 0, 10s, 10s, 10s, 10s, p, journal_));
    }

    void
    testCheckConsensus()
    {
        using namespace std::chrono_literals;

        ConsensusParms const p{};

        BEAST_EXPECT(
            ConsensusState::No ==
            checkConsensus(10, 2, 2, 0, 3s, 2s, p, true, journal_));

        BEAST_EXPECT(
            ConsensusState::No ==
            checkConsensus(10, 2, 2, 0, 3s, 4s, p, true, journal_));

        BEAST_EXPECT(
            ConsensusState::Yes ==
            checkConsensus(10, 2, 2, 0, 3s, 10s, p, true, journal_));

        BEAST_EXPECT(
            ConsensusState::No ==
            checkConsensus(10, 2, 1, 0, 3s, 10s, p, true, journal_));

        BEAST_EXPECT(
            ConsensusState::MovedOn ==
            checkConsensus(10, 2, 1, 8, 3s, 10s, p, true, journal_));

        BEAST_EXPECT(
            ConsensusState::Yes ==
            checkConsensus(0, 0, 0, 0, 3s, 10s, p, true, journal_));
    }

    void
    testStandalone()
    {
        using namespace std::chrono_literals;
        using namespace csf;

        Sim s;
        PeerGroup peers = s.createGroup(1);
        Peer * peer = peers[0];
        peer->targetLedgers = 1;
        peer->start();
        peer->submit(Tx{1});

        s.scheduler.step();

        auto const& lcl = peer->lastClosedLedger;
        BEAST_EXPECT(peer->prevLedgerID() == lcl.id());
        BEAST_EXPECT(lcl.seq() == Ledger::Seq{1});
        BEAST_EXPECT(lcl.txs().size() == 1);
        BEAST_EXPECT(lcl.txs().find(Tx{1}) != lcl.txs().end());
        BEAST_EXPECT(peer->prevProposers == 0);
    }

    void
    testPeersAgree()
    {
        using namespace csf;
        using namespace std::chrono;

        ConsensusParms const parms{};
        Sim sim;
        PeerGroup peers = sim.createGroup(5);

        peers.trustAndConnect(
            peers, date::round<milliseconds>(0.2 * parms.ledgerGRANULARITY));

        for (Peer * p : peers)
            p->submit(Tx(static_cast<std::uint32_t>(p->id)));

        sim.run(1);

        if (BEAST_EXPECT(sim.synchronized()))
        {
            for (Peer const* peer : peers)
            {
                auto const& lcl = peer->lastClosedLedger;
                BEAST_EXPECT(lcl.id() == peer->prevLedgerID());
                BEAST_EXPECT(lcl.seq() == Ledger::Seq{1});
                BEAST_EXPECT(peer->prevProposers == peers.size() - 1);
                for (std::uint32_t i = 0; i < peers.size(); ++i)
                    BEAST_EXPECT(lcl.txs().find(Tx{i}) != lcl.txs().end());
            }
        }
    }

    void
    testSlowPeers()
    {
        using namespace csf;
        using namespace std::chrono;


        {
            ConsensusParms const parms{};
            Sim sim;
            PeerGroup slow = sim.createGroup(1);
            PeerGroup fast = sim.createGroup(4);
            PeerGroup network = fast + slow;

            network.trust(network);

            fast.connect(
                fast, date::round<milliseconds>(0.2 * parms.ledgerGRANULARITY));

            slow.connect(
                network,
                date::round<milliseconds>(1.1 * parms.ledgerGRANULARITY));

            for (Peer* peer : network)
                peer->submit(Tx{static_cast<std::uint32_t>(peer->id)});

            sim.run(1);

            if (BEAST_EXPECT(sim.synchronized()))
            {
                for (Peer* peer : network)
                {
                    auto const& lcl = peer->lastClosedLedger;
                    BEAST_EXPECT(lcl.id() == peer->prevLedgerID());
                    BEAST_EXPECT(lcl.seq() == Ledger::Seq{1});

                    BEAST_EXPECT(peer->prevProposers == network.size() - 1);
                    BEAST_EXPECT(
                        peer->prevRoundTime == network[0]->prevRoundTime);

                    BEAST_EXPECT(lcl.txs().find(Tx{0}) == lcl.txs().end());
                    for (std::uint32_t i = 2; i < network.size(); ++i)
                        BEAST_EXPECT(lcl.txs().find(Tx{i}) != lcl.txs().end());

                    BEAST_EXPECT(
                        peer->openTxs.find(Tx{0}) != peer->openTxs.end());
                }
            }
        }

        {

            for (auto isParticipant : {true, false})
            {
                ConsensusParms const parms{};

                Sim sim;
                PeerGroup slow = sim.createGroup(2);
                PeerGroup fast = sim.createGroup(4);
                PeerGroup network = fast + slow;

                network.trust(network);

                fast.connect(
                    fast,
                    date::round<milliseconds>(0.2 * parms.ledgerGRANULARITY));

                slow.connect(
                    network,
                    date::round<milliseconds>(1.1 * parms.ledgerGRANULARITY));

                for (Peer* peer : slow)
                    peer->runAsValidator = isParticipant;

                for (Peer* peer : network)
                    peer->submit(Tx{static_cast<std::uint32_t>(peer->id)});

                sim.run(1);

                if (BEAST_EXPECT(sim.synchronized()))
                {
                    for (Peer* peer : network)
                    {
                        auto const& lcl = peer->lastClosedLedger;
                        BEAST_EXPECT(lcl.seq() == Ledger::Seq{1});
                        BEAST_EXPECT(lcl.txs().find(Tx{0}) == lcl.txs().end());
                        BEAST_EXPECT(lcl.txs().find(Tx{1}) == lcl.txs().end());
                        for (std::uint32_t i = slow.size(); i < network.size();
                             ++i)
                            BEAST_EXPECT(
                                lcl.txs().find(Tx{i}) != lcl.txs().end());

                        BEAST_EXPECT(
                            peer->openTxs.find(Tx{0}) != peer->openTxs.end());
                        BEAST_EXPECT(
                            peer->openTxs.find(Tx{1}) != peer->openTxs.end());
                    }

                    Peer const* slowPeer = slow[0];
                    if (isParticipant)
                        BEAST_EXPECT(
                            slowPeer->prevProposers == network.size() - 1);
                    else
                        BEAST_EXPECT(slowPeer->prevProposers == fast.size());

                    for (Peer* peer : fast)
                    {

                        if (isParticipant)
                        {
                            BEAST_EXPECT(
                                peer->prevProposers == network.size() - 1);
                            BEAST_EXPECT(
                                peer->prevRoundTime > slowPeer->prevRoundTime);
                        }
                        else
                        {
                            BEAST_EXPECT(
                                peer->prevProposers == fast.size() - 1);
                            BEAST_EXPECT(
                                peer->prevRoundTime == slowPeer->prevRoundTime);
                        }
                    }
                }

            }
        }
    }

    void
    testCloseTimeDisagree()
    {
        using namespace csf;
        using namespace std::chrono;






        ConsensusParms const parms{};
        Sim sim;

        PeerGroup groupA = sim.createGroup(2);
        PeerGroup groupB = sim.createGroup(2);
        PeerGroup groupC = sim.createGroup(2);
        PeerGroup network = groupA + groupB + groupC;

        network.trust(network);
        network.connect(
            network, date::round<milliseconds>(0.2 * parms.ledgerGRANULARITY));

        Peer* firstPeer = *groupA.begin();
        while (firstPeer->lastClosedLedger.closeTimeResolution() >=
               parms.proposeFRESHNESS)
            sim.run(1);

        for (Peer* peer : groupA)
            peer->clockSkew = parms.proposeFRESHNESS / 2;
        for (Peer* peer : groupB)
            peer->clockSkew = parms.proposeFRESHNESS;

        sim.run(1);

        if (BEAST_EXPECT(sim.synchronized()))
        {
            for (Peer* peer : network)
                BEAST_EXPECT(!peer->lastClosedLedger.closeAgree());
        }
    }

    void
    testWrongLCL()
    {
        using namespace csf;
        using namespace std::chrono;

        ConsensusParms const parms{};

        for (auto validationDelay : {0ms, parms.ledgerMIN_CLOSE})
        {




            Sim sim;

            PeerGroup minority = sim.createGroup(2);
            PeerGroup majorityA = sim.createGroup(3);
            PeerGroup majorityB = sim.createGroup(5);

            PeerGroup majority = majorityA + majorityB;
            PeerGroup network = minority + majority;

            SimDuration delay =
                date::round<milliseconds>(0.2 * parms.ledgerGRANULARITY);
            minority.trustAndConnect(minority + majorityA, delay);
            majority.trustAndConnect(majority, delay);

            CollectByNode<JumpCollector> jumps;
            sim.collectors.add(jumps);

            BEAST_EXPECT(sim.trustGraph.canFork(parms.minCONSENSUS_PCT / 100.));

            sim.run(1);

            for (Peer* peer : network)
                peer->delays.recvValidation = validationDelay;
            for (Peer* peer : (minority + majorityA))
                peer->openTxs.insert(Tx{0});
            for (Peer* peer : majorityB)
                peer->openTxs.insert(Tx{1});

            sim.run(3);


            if (BEAST_EXPECT(sim.branches() == 1))
            {
                for(Peer const* peer : majority)
                {
                    BEAST_EXPECT(jumps[peer->id].closeJumps.empty());
                    BEAST_EXPECT(jumps[peer->id].fullyValidatedJumps.empty());
                }
                for(Peer const* peer : minority)
                {
                    auto & peerJumps = jumps[peer->id];
                    {
                        if (BEAST_EXPECT(peerJumps.closeJumps.size() == 1))
                        {
                            JumpCollector::Jump const& jump =
                                peerJumps.closeJumps.front();
                            BEAST_EXPECT(jump.from.seq() <= jump.to.seq());
                            BEAST_EXPECT(!jump.to.isAncestor(jump.from));
                        }
                    }
                    {
                        if (BEAST_EXPECT(
                                peerJumps.fullyValidatedJumps.size() == 1))
                        {
                            JumpCollector::Jump const& jump =
                                peerJumps.fullyValidatedJumps.front();
                            BEAST_EXPECT(jump.from.seq() < jump.to.seq());
                            BEAST_EXPECT(jump.to.isAncestor(jump.from));
                        }
                    }
                }
            }
        }

        {


            Sim sim;
            PeerGroup loner = sim.createGroup(1);
            PeerGroup friends = sim.createGroup(3);
            loner.trust(loner + friends);

            PeerGroup others = sim.createGroup(6);
            PeerGroup clique = friends + others;
            clique.trust(clique);

            PeerGroup network = loner + clique;
            network.connect(
                network,
                date::round<milliseconds>(0.2 * parms.ledgerGRANULARITY));

            sim.run(1);
            for (Peer* peer : (loner + friends))
                peer->openTxs.insert(Tx(0));
            for (Peer* peer : others)
                peer->openTxs.insert(Tx(1));

            for (Peer* peer : network)
                peer->delays.recvValidation = parms.ledgerGRANULARITY;

            sim.run(2);

            for (Peer * p: network)
                BEAST_EXPECT(p->prevLedgerID() == network[0]->prevLedgerID());
        }
    }

    void
    testConsensusCloseTimeRounding()
    {
        using namespace csf;
        using namespace std::chrono;


        for (bool useRoundedCloseTime : {false, true})
        {
            ConsensusParms parms;
            parms.useRoundedCloseTime = useRoundedCloseTime;

            Sim sim;

            PeerGroup slow = sim.createGroup(2);
            PeerGroup fast = sim.createGroup(4);
            PeerGroup network = fast + slow;

            for (Peer* peer : network)
                peer->consensusParms = parms;

            network.trust(network);

            fast.connect(
                fast, date::round<milliseconds>(0.2 * parms.ledgerGRANULARITY));
            slow.connect(
                network,
                date::round<milliseconds>(1.1 * parms.ledgerGRANULARITY));

            sim.run(increaseLedgerTimeResolutionEvery - 2);



            NetClock::duration when = network[0]->now().time_since_epoch();

            NetClock::duration resolution =
                network[0]->lastClosedLedger.closeTimeResolution();
            BEAST_EXPECT(resolution == NetClock::duration{30s});

            while (
                ((when % NetClock::duration{30s}) != NetClock::duration{15s}) ||
                ((when % NetClock::duration{20s}) != NetClock::duration{15s}))
                when += 1s;
            sim.scheduler.step_for(
                NetClock::time_point{when} - network[0]->now());

            sim.run(1);
            if (BEAST_EXPECT(sim.synchronized()))
            {
                for (Peer* peer : network)
                {
                    BEAST_EXPECT(
                        peer->lastClosedLedger.closeTime() > peer->now());
                    BEAST_EXPECT(peer->lastClosedLedger.closeAgree());
                }
            }

            for (Peer* peer : network)
                peer->submit(Tx{static_cast<std::uint32_t>(peer->id)});



            sim.run(1);

            if (parms.useRoundedCloseTime)
            {
                BEAST_EXPECT(sim.synchronized());
            }
            else
            {
                BEAST_EXPECT(!sim.synchronized());

                BEAST_EXPECT(std::all_of(
                    slow.begin(), slow.end(), [&slow](Peer const* p) {
                        return p->lastClosedLedger.id() ==
                            slow[0]->lastClosedLedger.id();
                    }));

                BEAST_EXPECT(std::all_of(
                    fast.begin(), fast.end(), [&fast](Peer const* p) {
                        return p->lastClosedLedger.id() ==
                            fast[0]->lastClosedLedger.id();
                    }));

                Ledger const& slowLCL = slow[0]->lastClosedLedger;
                Ledger const& fastLCL = fast[0]->lastClosedLedger;

                BEAST_EXPECT(
                    slowLCL.parentCloseTime() == fastLCL.parentCloseTime());
                BEAST_EXPECT(
                    slowLCL.closeTimeResolution() ==
                    fastLCL.closeTimeResolution());

                BEAST_EXPECT(slowLCL.closeTime() != fastLCL.closeTime());
                BEAST_EXPECT(
                    effCloseTime(
                        slowLCL.closeTime(),
                        slowLCL.closeTimeResolution(),
                        slowLCL.parentCloseTime()) ==
                    effCloseTime(
                        fastLCL.closeTime(),
                        fastLCL.closeTimeResolution(),
                        fastLCL.parentCloseTime()));
            }
        }
    }

    void
    testFork()
    {
        using namespace csf;
        using namespace std::chrono;

        std::uint32_t numPeers = 10;
        for (std::uint32_t overlap = 0; overlap <= numPeers; ++overlap)
        {
            ConsensusParms const parms{};
            Sim sim;

            std::uint32_t numA = (numPeers - overlap) / 2;
            std::uint32_t numB = numPeers - numA - overlap;

            PeerGroup aOnly = sim.createGroup(numA);
            PeerGroup bOnly = sim.createGroup(numB);
            PeerGroup commonOnly = sim.createGroup(overlap);

            PeerGroup a = aOnly + commonOnly;
            PeerGroup b = bOnly + commonOnly;

            PeerGroup network = a + b;

            SimDuration delay =
                date::round<milliseconds>(0.2 * parms.ledgerGRANULARITY);
            a.trustAndConnect(a, delay);
            b.trustAndConnect(b, delay);

            sim.run(1);
            for (Peer* peer : network)
            {
                peer->openTxs.insert(Tx{static_cast<std::uint32_t>(peer->id)});
                for (Peer* to : sim.trustGraph.trustedPeers(peer))
                    peer->openTxs.insert(
                        Tx{static_cast<std::uint32_t>(to->id)});
            }
            sim.run(1);

            if (overlap > 0.4 * numPeers)
                BEAST_EXPECT(sim.synchronized());
            else
            {
                BEAST_EXPECT(sim.branches() <= 3);
            }
        }
    }

    void
    testHubNetwork()
    {
        using namespace csf;
        using namespace std::chrono;


        ConsensusParms const parms{};
        Sim sim;
        PeerGroup validators = sim.createGroup(5);
        PeerGroup center = sim.createGroup(1);
        validators.trust(validators);
        center.trust(validators);

        SimDuration delay =
                date::round<milliseconds>(0.2 * parms.ledgerGRANULARITY);
        validators.connect(center, delay);

        center[0]->runAsValidator = false;

        sim.run(1);

        for (Peer * p : validators)
            p->submit(Tx(static_cast<std::uint32_t>(p->id)));

        sim.run(1);

        BEAST_EXPECT(sim.synchronized());
    }


    struct Disruptor
    {
        csf::PeerGroup& network;
        csf::PeerGroup& groupCfast;
        csf::PeerGroup& groupCsplit;
        csf::SimDuration delay;
        bool reconnected = false;

        Disruptor(
            csf::PeerGroup& net,
            csf::PeerGroup& c,
            csf::PeerGroup& split,
            csf::SimDuration d)
            : network(net), groupCfast(c), groupCsplit(split), delay(d)
        {
        }

        template <class E>
        void
        on(csf::PeerID, csf::SimTime, E const&)
        {
        }


        void
        on(csf::PeerID who, csf::SimTime, csf::FullyValidateLedger const& e)
        {
            using namespace std::chrono;
            if (who == groupCfast[0]->id &&
                e.ledger.seq() == csf::Ledger::Seq{2})
            {
                network.disconnect(groupCsplit);
                network.disconnect(groupCfast);
            }
        }

        void
        on(csf::PeerID who, csf::SimTime, csf::AcceptLedger const& e)
        {
            if (!reconnected && e.ledger.seq() == csf::Ledger::Seq{3})
            {
                reconnected = true;
                network.connect(groupCsplit, delay);
            }
        }


    };

    void
    testPreferredByBranch()
    {
        using namespace csf;
        using namespace std::chrono;





        ConsensusParms const parms{};
        Sim sim;

        PeerGroup groupABD = sim.createGroup(2);
        PeerGroup groupCfast = sim.createGroup(1);
        PeerGroup groupCsplit = sim.createGroup(7);

        PeerGroup groupNotFastC = groupABD + groupCsplit;
        PeerGroup network = groupABD + groupCsplit + groupCfast;

        SimDuration delay = date::round<milliseconds>(
            0.2 * parms.ledgerGRANULARITY);
        SimDuration fDelay = date::round<milliseconds>(
            0.1 * parms.ledgerGRANULARITY);

        network.trust(network);
        network.connect(groupCfast, fDelay);
        groupNotFastC.connect(groupNotFastC, delay);

        Disruptor dc(network, groupCfast, groupCsplit, delay);
        sim.collectors.add(dc);

        sim.run(1);
        BEAST_EXPECT(sim.synchronized());

        for(Peer * peer : groupABD)
        {
            peer->txInjections.emplace(
                    peer->lastClosedLedger.seq(), Tx{42});
        }
        sim.run(1);

        BEAST_EXPECT(!sim.synchronized());
        BEAST_EXPECT(sim.branches() == 1);

        for (Peer * p : network)
            p->submit(Tx(static_cast<std::uint32_t>(p->id)));
        sim.run(1);

        BEAST_EXPECT(!sim.synchronized());
        BEAST_EXPECT(sim.branches() == 1);

        sim.run(1);

        if(BEAST_EXPECT(sim.branches() == 1))
        {
            BEAST_EXPECT(sim.synchronized());
        }
        else 
        {
            BEAST_EXPECT(sim.branches(groupNotFastC) == 1);
            BEAST_EXPECT(sim.synchronized(groupNotFastC) == 1);
        }
    }

    struct UndoDelay
    {
        csf::PeerGroup& g;

        UndoDelay(csf::PeerGroup& a) : g(a)
        {
        }

        template <class E>
        void
        on(csf::PeerID, csf::SimTime, E const&)
        {
        }

        void
        on(csf::PeerID who, csf::SimTime, csf::AcceptLedger const& e)
        {
            for (csf::Peer* p : g)
            {
                if (p->id == who)
                    p->delays.ledgerAccept = std::chrono::seconds{0};
            }
        }
    };

    void
    testPauseForLaggards()
    {
        using namespace csf;
        using namespace std::chrono;



        ConsensusParms const parms{};
        Sim sim;
        SimDuration delay =
            date::round<milliseconds>(0.2 * parms.ledgerGRANULARITY);

        PeerGroup behind = sim.createGroup(3);
        PeerGroup ahead = sim.createGroup(2);
        PeerGroup network = ahead + behind;

        hash_set<Peer::NodeKey_t> trustedKeys;
        for (Peer* p : network)
            trustedKeys.insert(p->key);
        for (Peer* p : network)
            p->trustedKeys = trustedKeys;

        network.trustAndConnect(network, delay);

        sim.run(1);

        for (Peer* p : behind)
            p->delays.ledgerAccept = 20s;

        UndoDelay undoDelay{behind};
        sim.collectors.add(undoDelay);

#if 0
        for (Peer* p : network)
            p->sink.threshold(beast::severities::kAll);

        StreamCollector sc{std::cout};
        sim.collectors.add(sc);
#endif
        std::chrono::nanoseconds const simDuration = 100s;

        Rate const rate{1, 5s};
        auto peerSelector = makeSelector(
            network.begin(),
            network.end(),
            std::vector<double>(network.size(), 1.),
            sim.rng);
        auto txSubmitter = makeSubmitter(
            ConstantDistribution{rate.inv()},
            sim.scheduler.now(),
            sim.scheduler.now() + simDuration,
            peerSelector,
            sim.scheduler,
            sim.rng);

        sim.run(simDuration);

        BEAST_EXPECT(sim.synchronized());
    }

    void
    run() override
    {
        testShouldCloseLedger();
        testCheckConsensus();

        testStandalone();
        testPeersAgree();
        testSlowPeers();
        testCloseTimeDisagree();
        testWrongLCL();
        testConsensusCloseTimeRounding();
        testFork();
        testHubNetwork();
        testPreferredByBranch();
        testPauseForLaggards();
    }
};

BEAST_DEFINE_TESTSUITE(Consensus, consensus, ripple);
}  
}  
























