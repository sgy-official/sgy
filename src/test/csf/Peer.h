
#ifndef RIPPLE_TEST_CSF_PEER_H_INCLUDED
#define RIPPLE_TEST_CSF_PEER_H_INCLUDED

#include <ripple/beast/utility/WrappedSink.h>
#include <ripple/consensus/Consensus.h>
#include <ripple/consensus/Validations.h>
#include <ripple/protocol/PublicKey.h>
#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <algorithm>
#include <test/csf/CollectorRef.h>
#include <test/csf/Scheduler.h>
#include <test/csf/TrustGraph.h>
#include <test/csf/Tx.h>
#include <test/csf/Validation.h>
#include <test/csf/events.h>
#include <test/csf/ledgers.h>

namespace ripple {
namespace test {
namespace csf {

namespace bc = boost::container;


struct Peer
{
    
    class Position
    {
    public:
        Position(Proposal const& p) : proposal_(p)
        {
        }

        Proposal const&
        proposal() const
        {
            return proposal_;
        }

        Json::Value
        getJson() const
        {
            return proposal_.getJson();
        }

    private:
        Proposal proposal_;
    };


    
    struct ProcessingDelays
    {
        std::chrono::milliseconds ledgerAccept{0};

        std::chrono::milliseconds recvValidation{0};

        template <class M>
        SimDuration
        onReceive(M const&) const
        {
            return SimDuration{};
        }

        SimDuration
        onReceive(Validation const&) const
        {
            return recvValidation;
        }
    };

    
    class ValAdaptor
    {
        Peer& p_;

    public:
        struct Mutex
        {
            void
            lock()
            {
            }

            void
            unlock()
            {
            }
        };

        using Validation = csf::Validation;
        using Ledger = csf::Ledger;

        ValAdaptor(Peer& p) : p_{p}
        {
        }

        NetClock::time_point
        now() const
        {
            return p_.now();
        }

        void
        onStale(Validation&& v)
        {
        }

        void
        flush(hash_map<PeerID, Validation>&& remaining)
        {
        }

        boost::optional<Ledger>
        acquire(Ledger::ID const & id)
        {
            if(Ledger const * ledger = p_.acquireLedger(id))
                return *ledger;
            return boost::none;
        }
    };

    using Ledger_t = Ledger;
    using NodeID_t = PeerID;
    using NodeKey_t = PeerKey;
    using TxSet_t = TxSet;
    using PeerPosition_t = Position;
    using Result = ConsensusResult<Peer>;
    using NodeKey = Validation::NodeKey;

    beast::WrappedSink sink;
    beast::Journal j;

    Consensus<Peer> consensus;

    PeerID id;

    PeerKey key;

    LedgerOracle& oracle;

    Scheduler& scheduler;

    BasicNetwork<Peer*>& net;

    TrustGraph<Peer*>& trustGraph;

    TxSetType openTxs;

    Ledger lastClosedLedger;

    hash_map<Ledger::ID, Ledger> ledgers;

    Validations<ValAdaptor> validations;

    Ledger fullyValidatedLedger;


    bc::flat_map<Ledger::ID, std::vector<Proposal>> peerPositions;
    bc::flat_map<TxSet::ID, TxSet> txSets;

    bc::flat_map<Ledger::ID,SimTime> acquiringLedgers;
    bc::flat_map<TxSet::ID,SimTime> acquiringTxSets;

    int completedLedgers = 0;

    int targetLedgers = std::numeric_limits<int>::max();

    std::chrono::seconds clockSkew{0};

    ProcessingDelays delays;

    bool runAsValidator = true;

    std::size_t prevProposers = 0;
    std::chrono::milliseconds prevRoundTime;

    std::size_t quorum = 0;

    hash_set<NodeKey_t> trustedKeys;

    ConsensusParms consensusParms;

    CollectorRefs & collectors;

    
    Peer(
        PeerID i,
        Scheduler& s,
        LedgerOracle& o,
        BasicNetwork<Peer*>& n,
        TrustGraph<Peer*>& tg,
        CollectorRefs& c,
        beast::Journal jIn)
        : sink(jIn, "Peer " + to_string(i) + ": ")
        , j(sink)
        , consensus(s.clock(), *this, j)
        , id{i}
        , key{id, 0}
        , oracle{o}
        , scheduler{s}
        , net{n}
        , trustGraph(tg)
        , lastClosedLedger{Ledger::MakeGenesis{}}
        , validations{ValidationParms{}, s.clock(), *this}
        , fullyValidatedLedger{Ledger::MakeGenesis{}}
        , collectors{c}
    {
        ledgers[lastClosedLedger.id()] = lastClosedLedger;

        trustGraph.trust(this, this);
    }

    
    template <class T>
    void
    schedule(std::chrono::nanoseconds when, T&& what)
    {
        using namespace std::chrono_literals;

        if (when == 0ns)
            what();
        else
            scheduler.in(when, std::forward<T>(what));
    }

    template <class E>
    void
    issue(E const & event)
    {
        collectors.on(id, scheduler.now(), event);
    }


    void
    trust(Peer & o)
    {
        trustGraph.trust(this, &o);
    }

    void
    untrust(Peer & o)
    {
        trustGraph.untrust(this, &o);
    }

    bool
    trusts(Peer & o)
    {
        return trustGraph.trusts(this, &o);
    }

    bool
    trusts(PeerID const & oId)
    {
        for(auto const & p : trustGraph.trustedPeers(this))
            if(p->id == oId)
                return true;
        return false;
    }

    

    bool
    connect(Peer & o, SimDuration dur)
    {
        return net.connect(this, &o, dur);
    }

    
    bool
    disconnect(Peer & o)
    {
        return net.disconnect(this, &o);
    }


    Ledger const*
    acquireLedger(Ledger::ID const& ledgerID)
    {
        auto it = ledgers.find(ledgerID);
        if (it != ledgers.end())
            return &(it->second);

        if(net.links(this).empty())
            return nullptr;

        auto aIt = acquiringLedgers.find(ledgerID);
        if(aIt!= acquiringLedgers.end())
        {
            if(scheduler.now() < aIt->second)
                return nullptr;
        }

        using namespace std::chrono_literals;
        SimDuration minDuration{10s};
        for (auto const& link : net.links(this))
        {
            minDuration = std::min(minDuration, link.data.delay);

            net.send(
                this, link.target, [ to = link.target, from = this, ledgerID ]() {
                    auto it = to->ledgers.find(ledgerID);
                    if (it != to->ledgers.end())
                    {
                        to->net.send(to, from, [ from, ledger = it->second ]() {
                            from->acquiringLedgers.erase(ledger.id());
                            from->ledgers.emplace(ledger.id(), ledger);
                        });
                    }
                });
        }
        acquiringLedgers[ledgerID] = scheduler.now() + 2 * minDuration;
        return nullptr;
    }

    TxSet const*
    acquireTxSet(TxSet::ID const& setId)
    {
        auto it = txSets.find(setId);
        if (it != txSets.end())
            return &(it->second);

        if(net.links(this).empty())
            return nullptr;

        auto aIt = acquiringTxSets.find(setId);
        if(aIt!= acquiringTxSets.end())
        {
            if(scheduler.now() < aIt->second)
                return nullptr;
        }

        using namespace std::chrono_literals;
        SimDuration minDuration{10s};
        for (auto const& link : net.links(this))
        {
            minDuration = std::min(minDuration, link.data.delay);
            net.send(
                this, link.target, [ to = link.target, from = this, setId ]() {
                    auto it = to->txSets.find(setId);
                    if (it != to->txSets.end())
                    {
                        to->net.send(to, from, [ from, txSet = it->second ]() {
                            from->acquiringTxSets.erase(txSet.id());
                            from->handle(txSet);
                        });
                    }
                });
        }
        acquiringTxSets[setId] = scheduler.now() + 2 * minDuration;
        return nullptr;
    }

    bool
    hasOpenTransactions() const
    {
        return !openTxs.empty();
    }

    std::size_t
    proposersValidated(Ledger::ID const& prevLedger)
    {
        return validations.numTrustedForLedger(prevLedger);
    }

    std::size_t
    proposersFinished(Ledger const & prevLedger, Ledger::ID const& prevLedgerID)
    {
        return validations.getNodesAfter(prevLedger, prevLedgerID);
    }

    Result
    onClose(
        Ledger const& prevLedger,
        NetClock::time_point closeTime,
        ConsensusMode mode)
    {
        issue(CloseLedger{prevLedger, openTxs});

        return Result(
            TxSet{openTxs},
            Proposal(
                prevLedger.id(),
                Proposal::seqJoin,
                TxSet::calcID(openTxs),
                closeTime,
                now(),
                id));
    }

    void
    onForceAccept(
        Result const& result,
        Ledger const& prevLedger,
        NetClock::duration const& closeResolution,
        ConsensusCloseTimes const& rawCloseTimes,
        ConsensusMode const& mode,
        Json::Value&& consensusJson)
    {
        onAccept(
            result,
            prevLedger,
            closeResolution,
            rawCloseTimes,
            mode,
            std::move(consensusJson));
    }

    void
    onAccept(
        Result const& result,
        Ledger const& prevLedger,
        NetClock::duration const& closeResolution,
        ConsensusCloseTimes const& rawCloseTimes,
        ConsensusMode const& mode,
        Json::Value&& consensusJson)
    {
        schedule(delays.ledgerAccept, [=]() {
            const bool proposing = mode == ConsensusMode::proposing;
            const bool consensusFail = result.state == ConsensusState::MovedOn;

            TxSet const acceptedTxs = injectTxs(prevLedger, result.txns);
            Ledger const newLedger = oracle.accept(
                prevLedger,
                acceptedTxs.txs(),
                closeResolution,
                result.position.closeTime());
            ledgers[newLedger.id()] = newLedger;

            issue(AcceptLedger{newLedger, lastClosedLedger});
            prevProposers = result.proposers;
            prevRoundTime = result.roundTime.read();
            lastClosedLedger = newLedger;

            auto const it = std::remove_if(
                openTxs.begin(), openTxs.end(), [&](Tx const& tx) {
                    return acceptedTxs.exists(tx.id());
                });
            openTxs.erase(it, openTxs.end());

            bool const isCompatible =
                newLedger.isAncestor(fullyValidatedLedger);

            if (runAsValidator && isCompatible && !consensusFail &&
                validations.canValidateSeq(newLedger.seq()))
            {
                bool isFull = proposing;

                Validation v{newLedger.id(),
                             newLedger.seq(),
                             now(),
                             now(),
                             key,
                             id,
                             isFull};
                share(v);
                addTrustedValidation(v);
            }

            checkFullyValidated(newLedger);

            ++completedLedgers;
            if (completedLedgers <= targetLedgers)
            {
                startRound();
            }
        });
    }

    Ledger::Seq
    earliestAllowedSeq() const
    {
        return fullyValidatedLedger.seq();
    }

    Ledger::ID
    getPrevLedger(
        Ledger::ID const& ledgerID,
        Ledger const& ledger,
        ConsensusMode mode)
    {
        if (ledger.seq() == Ledger::Seq{0})
            return ledgerID;

        Ledger::ID const netLgr =
            validations.getPreferred(ledger, earliestAllowedSeq());

        if (netLgr != ledgerID)
        {
            JLOG(j.trace()) << Json::Compact(validations.getJsonTrie());
            issue(WrongPrevLedger{ledgerID, netLgr});
        }

        return netLgr;
    }

    void
    propose(Proposal const& pos)
    {
        share(pos);
    }

    ConsensusParms const&
    parms() const
    {
        return consensusParms;
    }

    void
    onModeChange(ConsensusMode, ConsensusMode)
    {
    }

    template <class M>
    void
    share(M const& m)
    {
        issue(Share<M>{m});
        send(BroadcastMesg<M>{m,router.nextSeq++, this->id}, this->id);
    }

    void
    share(Position const & p)
    {
        share(p.proposal());
    }


    
    bool
    addTrustedValidation(Validation v)
    {
        v.setTrusted();
        v.setSeen(now());
        ValStatus const res = validations.add(v.nodeID(), v);

        if(res == ValStatus::stale)
            return false;

        if (Ledger const* lgr = acquireLedger(v.ledgerID()))
            checkFullyValidated(*lgr);
        return true;
    }

    
    void
    checkFullyValidated(Ledger const& ledger)
    {
        if (ledger.seq() <= fullyValidatedLedger.seq())
            return;

        std::size_t const count = validations.numTrustedForLedger(ledger.id());
        std::size_t const numTrustedPeers = trustGraph.graph().outDegree(this);
        quorum = static_cast<std::size_t>(std::ceil(numTrustedPeers * 0.8));
        if (count >= quorum && ledger.isAncestor(fullyValidatedLedger))
        {
            issue(FullyValidateLedger{ledger, fullyValidatedLedger});
            fullyValidatedLedger = ledger;
        }
    }


    template <class M>
    struct BroadcastMesg
    {
        M mesg;
        std::size_t seq;
        PeerID origin;
    };

    struct Router
    {
        std::size_t nextSeq = 1;
        bc::flat_map<PeerID, std::size_t> lastObservedSeq;
    };

    Router router;

    template <class M>
    void
    send(BroadcastMesg<M> const& bm, PeerID from)
    {
        for (auto const& link : net.links(this))
        {
            if (link.target->id != from && link.target->id != bm.origin)
            {
                if (link.target->router.lastObservedSeq[bm.origin] < bm.seq)
                {
                    issue(Relay<M>{link.target->id, bm.mesg});
                    net.send(
                        this, link.target, [to = link.target, bm, id = this->id ] {
                            to->receive(bm, id);
                        });
                }
            }
        }
    }

    template <class M>
    void
    receive(BroadcastMesg<M> const& bm, PeerID from)
    {
        issue(Receive<M>{from, bm.mesg});
        if (router.lastObservedSeq[bm.origin] < bm.seq)
        {
            router.lastObservedSeq[bm.origin] = bm.seq;
            schedule(delays.onReceive(bm.mesg), [this, bm, from]
            {
                if (handle(bm.mesg))
                    send(bm, from);
            });
        }
    }

    bool
    handle(Proposal const& p)
    {
        if(!trusts(p.nodeID()))
            return p.prevLedger() == lastClosedLedger.id();

        auto& dest = peerPositions[p.prevLedger()];
        if (std::find(dest.begin(), dest.end(), p) != dest.end())
            return false;

        dest.push_back(p);

        return consensus.peerProposal(now(), Position{p});
    }

    bool
    handle(TxSet const& txs)
    {
        auto const it = txSets.insert(std::make_pair(txs.id(), txs));
        if (it.second)
            consensus.gotTxSet(now(), txs);
        return it.second;
    }

    bool
    handle(Tx const& tx)
    {
        TxSetType const& lastClosedTxs = lastClosedLedger.txs();
        if (lastClosedTxs.find(tx) != lastClosedTxs.end())
            return false;

        return openTxs.insert(tx).second;

    }

    bool
    handle(Validation const& v)
    {
        if (!trusts(v.nodeID()))
            return false;

        return addTrustedValidation(v);
    }

    bool
    haveValidated() const
    {
        return fullyValidatedLedger.seq() > Ledger::Seq{0};
    }

    Ledger::Seq
    getValidLedgerIndex() const
    {
        return earliestAllowedSeq();
    }

    std::pair<std::size_t, hash_set<NodeKey_t>>
    getQuorumKeys()
    {
        hash_set<NodeKey_t > keys;
        for (auto const& p : trustGraph.trustedPeers(this))
            keys.insert(p->key);
        return {quorum, keys};
    }

    std::size_t
    laggards(Ledger::Seq const seq, hash_set<NodeKey_t>& trustedKeys)
    {
        return validations.laggards(seq, trustedKeys);
    }

    bool
    validator() const
    {
        return runAsValidator;
    }

    void
    submit(Tx const& tx)
    {
        issue(SubmitTx{tx});
        if(handle(tx))
            share(tx);
    }


    void
    timerEntry()
    {
        consensus.timerEntry(now());
        if (completedLedgers < targetLedgers)
            scheduler.in(parms().ledgerGRANULARITY, [this]() { timerEntry(); });
    }

    void
    startRound()
    {
        Ledger::ID bestLCL =
            validations.getPreferred(lastClosedLedger, earliestAllowedSeq());
        if(bestLCL == Ledger::ID{0})
            bestLCL = lastClosedLedger.id();

        issue(StartRound{bestLCL, lastClosedLedger});

        hash_set<PeerID> nowUntrusted;
        consensus.startRound(
            now(), bestLCL, lastClosedLedger, nowUntrusted, runAsValidator);
    }

    void
    start()
    {
        validations.expire();
        scheduler.in(parms().ledgerGRANULARITY, [&]() { timerEntry(); });
        startRound();
    }

    NetClock::time_point
    now() const
    {
        using namespace std::chrono;
        using namespace std::chrono_literals;
        return NetClock::time_point(duration_cast<NetClock::duration>(
            scheduler.now().time_since_epoch() + 86400s + clockSkew));
    }


    Ledger::ID
    prevLedgerID() const
    {
        return consensus.prevLedgerID();
    }

    hash_map<Ledger::Seq, Tx> txInjections;

    
    TxSet
    injectTxs(Ledger prevLedger, TxSet const & src)
    {
        auto const it = txInjections.find(prevLedger.seq());

        if(it == txInjections.end())
            return src;
        TxSetType res{src.txs()};
        res.insert(it->second);

        return TxSet{res};

    }
};

}  
}  
}  
#endif









