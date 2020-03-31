

#ifndef RIPPLE_CONSENSUS_CONSENSUS_H_INCLUDED
#define RIPPLE_CONSENSUS_CONSENSUS_H_INCLUDED

#include <ripple/basics/Log.h>
#include <ripple/basics/chrono.h>
#include <ripple/beast/utility/Journal.h>
#include <ripple/consensus/ConsensusProposal.h>
#include <ripple/consensus/ConsensusParms.h>
#include <ripple/consensus/ConsensusTypes.h>
#include <ripple/consensus/LedgerTiming.h>
#include <ripple/consensus/DisputedTx.h>
#include <ripple/json/json_writer.h>
#include <boost/logic/tribool.hpp>
#include <sstream>

namespace ripple {



bool
shouldCloseLedger(
    bool anyTransactions,
    std::size_t prevProposers,
    std::size_t proposersClosed,
    std::size_t proposersValidated,
    std::chrono::milliseconds prevRoundTime,
    std::chrono::milliseconds timeSincePrevClose,
    std::chrono::milliseconds openTime,
    std::chrono::milliseconds idleInterval,
    ConsensusParms const & parms,
    beast::Journal j);


ConsensusState
checkConsensus(
    std::size_t prevProposers,
    std::size_t currentProposers,
    std::size_t currentAgree,
    std::size_t currentFinished,
    std::chrono::milliseconds previousAgreeTime,
    std::chrono::milliseconds currentAgreeTime,
    ConsensusParms const & parms,
    bool proposing,
    beast::Journal j);


template <class Adaptor>
class Consensus
{
    using Ledger_t = typename Adaptor::Ledger_t;
    using TxSet_t = typename Adaptor::TxSet_t;
    using NodeID_t = typename Adaptor::NodeID_t;
    using Tx_t = typename TxSet_t::Tx;
    using PeerPosition_t = typename Adaptor::PeerPosition_t;
    using Proposal_t = ConsensusProposal<
        NodeID_t,
        typename Ledger_t::ID,
        typename TxSet_t::ID>;

    using Result = ConsensusResult<Adaptor>;

    class MonitoredMode
    {
        ConsensusMode mode_;

    public:
        MonitoredMode(ConsensusMode m) : mode_{m}
        {
        }
        ConsensusMode
        get() const
        {
            return mode_;
        }

        void
        set(ConsensusMode mode, Adaptor& a)
        {
            a.onModeChange(mode_, mode);
            mode_ = mode;
        }
    };
public:
    using clock_type = beast::abstract_clock<std::chrono::steady_clock>;

    Consensus(Consensus&&) noexcept = default;

    
    Consensus(clock_type const& clock, Adaptor& adaptor, beast::Journal j);

    
    void
    startRound(
        NetClock::time_point const& now,
        typename Ledger_t::ID const& prevLedgerID,
        Ledger_t prevLedger,
        hash_set<NodeID_t> const & nowUntrusted,
        bool proposing);

    
    bool
    peerProposal(
        NetClock::time_point const& now,
        PeerPosition_t const& newProposal);

    
    void
    timerEntry(NetClock::time_point const& now);

    
    void
    gotTxSet(NetClock::time_point const& now, TxSet_t const& txSet);

    
    void
    simulate(
        NetClock::time_point const& now,
        boost::optional<std::chrono::milliseconds> consensusDelay);

    
    typename Ledger_t::ID
    prevLedgerID() const
    {
        return prevLedgerID_;
    }

    
    Json::Value
    getJson(bool full) const;

private:
    void
    startRoundInternal(
        NetClock::time_point const& now,
        typename Ledger_t::ID const& prevLedgerID,
        Ledger_t const& prevLedger,
        ConsensusMode mode);

    void
    handleWrongLedger(typename Ledger_t::ID const& lgrId);

    
    void
    checkLedger();

    
    void
    playbackProposals();

    
    bool
    peerProposalInternal(
        NetClock::time_point const& now,
        PeerPosition_t const& newProposal);

    
    void
    phaseOpen();

    
    void
    phaseEstablish();

    
    bool
    shouldPause() const;

    void
    closeLedger();

    void
    updateOurPositions();

    bool
    haveConsensus();

    void
    createDisputes(TxSet_t const& o);

    void
    updateDisputes(NodeID_t const& node, TxSet_t const& other);

    void
    leaveConsensus();

    NetClock::time_point
    asCloseTime(NetClock::time_point raw) const;

private:
    Adaptor& adaptor_;

    ConsensusPhase phase_{ConsensusPhase::accepted};
    MonitoredMode mode_{ConsensusMode::observing};
    bool firstRound_ = true;
    bool haveCloseTimeConsensus_ = false;

    clock_type const& clock_;

    int convergePercent_{0};

    ConsensusTimer openTime_;

    NetClock::duration closeResolution_ = ledgerDefaultTimeResolution;

    std::chrono::milliseconds prevRoundTime_;


    NetClock::time_point now_;
    NetClock::time_point prevCloseTime_;


    typename Ledger_t::ID prevLedgerID_;
    Ledger_t previousLedger_;

    hash_map<typename TxSet_t::ID, const TxSet_t> acquired_;

    boost::optional<Result> result_;
    ConsensusCloseTimes rawCloseTimes_;


    hash_map<NodeID_t, PeerPosition_t> currPeerPositions_;

    hash_map<NodeID_t, std::deque<PeerPosition_t>> recentPeerPositions_;

    std::size_t prevProposers_ = 0;

    hash_set<NodeID_t> deadNodes_;

    beast::Journal j_;
};

template <class Adaptor>
Consensus<Adaptor>::Consensus(
    clock_type const& clock,
    Adaptor& adaptor,
    beast::Journal journal)
    : adaptor_(adaptor)
    , clock_(clock)
    , j_{journal}
{
    JLOG(j_.debug()) << "Creating consensus object";
}

template <class Adaptor>
void
Consensus<Adaptor>::startRound(
    NetClock::time_point const& now,
    typename Ledger_t::ID const& prevLedgerID,
    Ledger_t prevLedger,
    hash_set<NodeID_t> const& nowUntrusted,
    bool proposing)
{
    if (firstRound_)
    {
        prevRoundTime_ = adaptor_.parms().ledgerIDLE_INTERVAL;
        prevCloseTime_ = prevLedger.closeTime();
        firstRound_ = false;
    }
    else
    {
        prevCloseTime_ = rawCloseTimes_.self;
    }

    for(NodeID_t const& n : nowUntrusted)
        recentPeerPositions_.erase(n);

    ConsensusMode startMode =
        proposing ? ConsensusMode::proposing : ConsensusMode::observing;

    if (prevLedger.id() != prevLedgerID)
    {
        if (auto newLedger = adaptor_.acquireLedger(prevLedgerID))
        {
            prevLedger = *newLedger;
        }
        else  
        {
            startMode = ConsensusMode::wrongLedger;
            JLOG(j_.info())
                << "Entering consensus with: " << previousLedger_.id();
            JLOG(j_.info()) << "Correct LCL is: " << prevLedgerID;
        }
    }

    startRoundInternal(now, prevLedgerID, prevLedger, startMode);
}
template <class Adaptor>
void
Consensus<Adaptor>::startRoundInternal(
    NetClock::time_point const& now,
    typename Ledger_t::ID const& prevLedgerID,
    Ledger_t const& prevLedger,
    ConsensusMode mode)
{
    phase_ = ConsensusPhase::open;
    mode_.set(mode, adaptor_);
    now_ = now;
    prevLedgerID_ = prevLedgerID;
    previousLedger_ = prevLedger;
    result_.reset();
    convergePercent_ = 0;
    haveCloseTimeConsensus_ = false;
    openTime_.reset(clock_.now());
    currPeerPositions_.clear();
    acquired_.clear();
    rawCloseTimes_.peers.clear();
    rawCloseTimes_.self = {};
    deadNodes_.clear();

    closeResolution_ = getNextLedgerTimeResolution(
        previousLedger_.closeTimeResolution(),
        previousLedger_.closeAgree(),
        previousLedger_.seq() + typename Ledger_t::Seq{1});

    playbackProposals();
    if (currPeerPositions_.size() > (prevProposers_ / 2))
    {
        timerEntry(now_);
    }
}

template <class Adaptor>
bool
Consensus<Adaptor>::peerProposal(
    NetClock::time_point const& now,
    PeerPosition_t const& newPeerPos)
{
    NodeID_t const& peerID = newPeerPos.proposal().nodeID();

    {
        auto& props = recentPeerPositions_[peerID];

        if (props.size() >= 10)
            props.pop_front();

        props.push_back(newPeerPos);
    }
    return peerProposalInternal(now, newPeerPos);
}

template <class Adaptor>
bool
Consensus<Adaptor>::peerProposalInternal(
    NetClock::time_point const& now,
    PeerPosition_t const& newPeerPos)
{
    if (phase_ == ConsensusPhase::accepted)
        return false;

    now_ = now;

    Proposal_t const& newPeerProp = newPeerPos.proposal();

    NodeID_t const& peerID = newPeerProp.nodeID();

    if (newPeerProp.prevLedger() != prevLedgerID_)
    {
        JLOG(j_.debug()) << "Got proposal for " << newPeerProp.prevLedger()
                         << " but we are on " << prevLedgerID_;
        return false;
    }

    if (deadNodes_.find(peerID) != deadNodes_.end())
    {
        using std::to_string;
        JLOG(j_.info()) << "Position from dead node: " << to_string(peerID);
        return false;
    }

    {
        auto peerPosIt = currPeerPositions_.find(peerID);

        if (peerPosIt != currPeerPositions_.end())
        {
            if (newPeerProp.proposeSeq() <=
                peerPosIt->second.proposal().proposeSeq())
            {
                return false;
            }
        }

        if (newPeerProp.isBowOut())
        {
            using std::to_string;

            JLOG(j_.info()) << "Peer bows out: " << to_string(peerID);
            if (result_)
            {
                for (auto& it : result_->disputes)
                    it.second.unVote(peerID);
            }
            if (peerPosIt != currPeerPositions_.end())
                currPeerPositions_.erase(peerID);
            deadNodes_.insert(peerID);

            return true;
        }

        if (peerPosIt != currPeerPositions_.end())
            peerPosIt->second = newPeerPos;
        else
            currPeerPositions_.emplace(peerID, newPeerPos);
    }

    if (newPeerProp.isInitial())
    {
        JLOG(j_.trace()) << "Peer reports close time as "
                         << newPeerProp.closeTime().time_since_epoch().count();
        ++rawCloseTimes_.peers[newPeerProp.closeTime()];
    }

    JLOG(j_.trace()) << "Processing peer proposal " << newPeerProp.proposeSeq()
                     << "/" << newPeerProp.position();

    {
        auto const ait = acquired_.find(newPeerProp.position());
        if (ait == acquired_.end())
        {
            if (auto set = adaptor_.acquireTxSet(newPeerProp.position()))
                gotTxSet(now_, *set);
            else
                JLOG(j_.debug()) << "Don't have tx set for peer";
        }
        else if (result_)
        {
            updateDisputes(newPeerProp.nodeID(), ait->second);
        }
    }

    return true;
}

template <class Adaptor>
void
Consensus<Adaptor>::timerEntry(NetClock::time_point const& now)
{
    if (phase_ == ConsensusPhase::accepted)
        return;

    now_ = now;

    checkLedger();

    if (phase_ == ConsensusPhase::open)
    {
        phaseOpen();
    }
    else if (phase_ == ConsensusPhase::establish)
    {
        phaseEstablish();
    }
}

template <class Adaptor>
void
Consensus<Adaptor>::gotTxSet(
    NetClock::time_point const& now,
    TxSet_t const& txSet)
{
    if (phase_ == ConsensusPhase::accepted)
        return;

    now_ = now;

    auto id = txSet.id();

    if (!acquired_.emplace(id, txSet).second)
        return;

    if (!result_)
    {
        JLOG(j_.debug()) << "Not creating disputes: no position yet.";
    }
    else
    {
        assert(id != result_->position.position());
        bool any = false;
        for (auto const& it : currPeerPositions_)
        {
            if (it.second.proposal().position() == id)
            {
                updateDisputes(it.first, txSet);
                any = true;
            }
        }

        if (!any)
        {
            JLOG(j_.warn())
                << "By the time we got " << id << " no peers were proposing it";
        }
    }
}

template <class Adaptor>
void
Consensus<Adaptor>::simulate(
    NetClock::time_point const& now,
    boost::optional<std::chrono::milliseconds> consensusDelay)
{
    using namespace std::chrono_literals;
    JLOG(j_.info()) << "Simulating consensus";
    now_ = now;
    closeLedger();
    result_->roundTime.tick(consensusDelay.value_or(100ms));
    result_->proposers = prevProposers_ = currPeerPositions_.size();
    prevRoundTime_ = result_->roundTime.read();
    phase_ = ConsensusPhase::accepted;
    adaptor_.onForceAccept(
        *result_,
        previousLedger_,
        closeResolution_,
        rawCloseTimes_,
        mode_.get(),
        getJson(true));
    JLOG(j_.info()) << "Simulation complete";
}

template <class Adaptor>
Json::Value
Consensus<Adaptor>::getJson(bool full) const
{
    using std::to_string;
    using Int = Json::Value::Int;

    Json::Value ret(Json::objectValue);

    ret["proposing"] = (mode_.get() == ConsensusMode::proposing);
    ret["proposers"] = static_cast<int>(currPeerPositions_.size());

    if (mode_.get() != ConsensusMode::wrongLedger)
    {
        ret["synched"] = true;
        ret["ledger_seq"] = static_cast<std::uint32_t>(previousLedger_.seq())+ 1;
        ret["close_granularity"] = static_cast<Int>(closeResolution_.count());
    }
    else
        ret["synched"] = false;

    ret["phase"] = to_string(phase_);

    if (result_ && !result_->disputes.empty() && !full)
        ret["disputes"] = static_cast<Int>(result_->disputes.size());

    if (result_)
        ret["our_position"] = result_->position.getJson();

    if (full)
    {
        if (result_)
            ret["current_ms"] =
                static_cast<Int>(result_->roundTime.read().count());
        ret["converge_percent"] = convergePercent_;
        ret["close_resolution"] = static_cast<Int>(closeResolution_.count());
        ret["have_time_consensus"] = haveCloseTimeConsensus_;
        ret["previous_proposers"] = static_cast<Int>(prevProposers_);
        ret["previous_mseconds"] = static_cast<Int>(prevRoundTime_.count());

        if (!currPeerPositions_.empty())
        {
            Json::Value ppj(Json::objectValue);

            for (auto const& pp : currPeerPositions_)
            {
                ppj[to_string(pp.first)] = pp.second.getJson();
            }
            ret["peer_positions"] = std::move(ppj);
        }

        if (!acquired_.empty())
        {
            Json::Value acq(Json::arrayValue);
            for (auto const& at : acquired_)
            {
                acq.append(to_string(at.first));
            }
            ret["acquired"] = std::move(acq);
        }

        if (result_ && !result_->disputes.empty())
        {
            Json::Value dsj(Json::objectValue);
            for (auto const& dt : result_->disputes)
            {
                dsj[to_string(dt.first)] = dt.second.getJson();
            }
            ret["disputes"] = std::move(dsj);
        }

        if (!rawCloseTimes_.peers.empty())
        {
            Json::Value ctj(Json::objectValue);
            for (auto const& ct : rawCloseTimes_.peers)
            {
                ctj[std::to_string(ct.first.time_since_epoch().count())] =
                    ct.second;
            }
            ret["close_times"] = std::move(ctj);
        }

        if (!deadNodes_.empty())
        {
            Json::Value dnj(Json::arrayValue);
            for (auto const& dn : deadNodes_)
            {
                dnj.append(to_string(dn));
            }
            ret["dead_nodes"] = std::move(dnj);
        }
    }

    return ret;
}

template <class Adaptor>
void
Consensus<Adaptor>::handleWrongLedger(typename Ledger_t::ID const& lgrId)
{
    assert(lgrId != prevLedgerID_ || previousLedger_.id() != lgrId);

    leaveConsensus();

    if (prevLedgerID_ != lgrId)
    {
        prevLedgerID_ = lgrId;

        if (result_)
        {
            result_->disputes.clear();
            result_->compares.clear();
        }

        currPeerPositions_.clear();
        rawCloseTimes_.peers.clear();
        deadNodes_.clear();

        playbackProposals();
    }

    if (previousLedger_.id() == prevLedgerID_)
        return;

    if (auto newLedger = adaptor_.acquireLedger(prevLedgerID_))
    {
        JLOG(j_.info()) << "Have the consensus ledger " << prevLedgerID_;
        startRoundInternal(
            now_, lgrId, *newLedger, ConsensusMode::switchedLedger);
    }
    else
    {
        mode_.set(ConsensusMode::wrongLedger, adaptor_);
    }
}

template <class Adaptor>
void
Consensus<Adaptor>::checkLedger()
{
    auto netLgr =
        adaptor_.getPrevLedger(prevLedgerID_, previousLedger_, mode_.get());

    if (netLgr != prevLedgerID_)
    {
        JLOG(j_.warn()) << "View of consensus changed during "
                        << to_string(phase_) << " status=" << to_string(phase_)
                        << ", "
                        << " mode=" << to_string(mode_.get());
        JLOG(j_.warn()) << prevLedgerID_ << " to " << netLgr;
        JLOG(j_.warn()) << Json::Compact{previousLedger_.getJson()};
        JLOG(j_.debug()) << "State on consensus change "
                         << Json::Compact{getJson(true)};
        handleWrongLedger(netLgr);
    }
    else if (previousLedger_.id() != prevLedgerID_)
        handleWrongLedger(netLgr);
}

template <class Adaptor>
void
Consensus<Adaptor>::playbackProposals()
{
    for (auto const& it : recentPeerPositions_)
    {
        for (auto const& pos : it.second)
        {
            if (pos.proposal().prevLedger() == prevLedgerID_)
            {
                if (peerProposalInternal(now_, pos))
                    adaptor_.share(pos);
            }
        }
    }
}

template <class Adaptor>
void
Consensus<Adaptor>::phaseOpen()
{
    using namespace std::chrono;

    bool anyTransactions = adaptor_.hasOpenTransactions();
    auto proposersClosed = currPeerPositions_.size();
    auto proposersValidated = adaptor_.proposersValidated(prevLedgerID_);

    openTime_.tick(clock_.now());

    milliseconds sinceClose;
    {
        bool previousCloseCorrect =
            (mode_.get() != ConsensusMode::wrongLedger) &&
            previousLedger_.closeAgree() &&
            (previousLedger_.closeTime() !=
             (previousLedger_.parentCloseTime() + 1s));

        auto lastCloseTime = previousCloseCorrect
            ? previousLedger_.closeTime()  
            : prevCloseTime_;              

        if (now_ >= lastCloseTime)
            sinceClose = duration_cast<milliseconds>(now_ - lastCloseTime);
        else
            sinceClose = -duration_cast<milliseconds>(lastCloseTime - now_);
    }

    auto const idleInterval = std::max<milliseconds>(
        adaptor_.parms().ledgerIDLE_INTERVAL,
        2 * previousLedger_.closeTimeResolution());

    if (shouldCloseLedger(
            anyTransactions,
            prevProposers_,
            proposersClosed,
            proposersValidated,
            prevRoundTime_,
            sinceClose,
            openTime_.read(),
            idleInterval,
            adaptor_.parms(),
            j_))
    {
        closeLedger();
    }
}

template <class Adaptor>
bool
Consensus<Adaptor>::shouldPause() const
{
    auto const& parms = adaptor_.parms();
    std::uint32_t const ahead (previousLedger_.seq() -
        std::min(adaptor_.getValidLedgerIndex(), previousLedger_.seq()));
    auto quorumKeys = adaptor_.getQuorumKeys();
    auto const& quorum = quorumKeys.first;
    auto& trustedKeys = quorumKeys.second;
    std::size_t const totalValidators = trustedKeys.size();
    std::size_t laggards = adaptor_.laggards(previousLedger_.seq(),
        trustedKeys);
    std::size_t const offline = trustedKeys.size();

    std::stringstream vars;
    vars << " (working seq: " << previousLedger_.seq() << ", "
         << "validated seq: " << adaptor_.getValidLedgerIndex() << ", "
         << "am validator: " << adaptor_.validator() << ", "
         << "have validated: " << adaptor_.haveValidated() << ", "
         << "roundTime: " << result_->roundTime.read().count() << ", "
         << "max consensus time: " << parms.ledgerMAX_CONSENSUS.count() << ", "
         << "validators: " << totalValidators << ", "
         << "laggards: " << laggards << ", "
         << "offline: " << offline << ", "
         << "quorum: " << quorum << ")";

    if (!ahead ||
        !laggards ||
        !totalValidators ||
        !adaptor_.validator() ||
        !adaptor_.haveValidated() ||
        result_->roundTime.read() > parms.ledgerMAX_CONSENSUS)
    {
        j_.debug() << "not pausing" << vars.str();
        return false;
    }

    bool willPause = false;

    
    constexpr static std::size_t maxPausePhase = 4;

    
    std::size_t const phase = (ahead - 1) % (maxPausePhase + 1);

    switch (phase)
    {
        case 0:
            if (laggards + offline > totalValidators - quorum)
                willPause = true;
            break;
        case maxPausePhase:
            willPause = true;
            break;
        default:
            float const nonLaggards = totalValidators - (laggards + offline);
            float const quorumRatio =
                static_cast<float>(quorum) / totalValidators;
            float const allowedDissent = 1.0f - quorumRatio;
            float const phaseFactor = static_cast<float>(phase) / maxPausePhase;

            if (nonLaggards / totalValidators <
                quorumRatio + (allowedDissent * phaseFactor))
            {
                willPause = true;
            }
    }

    if (willPause)
        j_.warn() << "pausing" << vars.str();
    else
        j_.debug() << "not pausing" << vars.str();
    return willPause;
}

template <class Adaptor>
void
Consensus<Adaptor>::phaseEstablish()
{
    assert(result_);

    using namespace std::chrono;
    ConsensusParms const & parms = adaptor_.parms();

    result_->roundTime.tick(clock_.now());
    result_->proposers = currPeerPositions_.size();

    convergePercent_ = result_->roundTime.read() * 100 /
        std::max<milliseconds>(prevRoundTime_, parms.avMIN_CONSENSUS_TIME);

    if (result_->roundTime.read() < parms.ledgerMIN_CONSENSUS)
        return;

    updateOurPositions();

    if (shouldPause() || !haveConsensus())
        return;

    if (!haveCloseTimeConsensus_)
    {
        JLOG(j_.info()) << "We have TX consensus but not CT consensus";
        return;
    }

    JLOG(j_.info()) << "Converge cutoff (" << currPeerPositions_.size()
                    << " participants)";
    prevProposers_ = currPeerPositions_.size();
    prevRoundTime_ = result_->roundTime.read();
    phase_ = ConsensusPhase::accepted;
    adaptor_.onAccept(
        *result_,
        previousLedger_,
        closeResolution_,
        rawCloseTimes_,
        mode_.get(),
        getJson(true));
}

template <class Adaptor>
void
Consensus<Adaptor>::closeLedger()
{
    assert(!result_);

    phase_ = ConsensusPhase::establish;
    rawCloseTimes_.self = now_;

    result_.emplace(adaptor_.onClose(previousLedger_, now_, mode_.get()));
    result_->roundTime.reset(clock_.now());
    if (acquired_.emplace(result_->txns.id(), result_->txns).second)
        adaptor_.share(result_->txns);

    if (mode_.get() == ConsensusMode::proposing)
        adaptor_.propose(result_->position);

    for (auto const& pit : currPeerPositions_)
    {
        auto const& pos = pit.second.proposal().position();
        auto const it = acquired_.find(pos);
        if (it != acquired_.end())
        {
            createDisputes(it->second);
        }
    }
}


inline int
participantsNeeded(int participants, int percent)
{
    int result = ((participants * percent) + (percent / 2)) / 100;

    return (result == 0) ? 1 : result;
}

template <class Adaptor>
void
Consensus<Adaptor>::updateOurPositions()
{
    assert(result_);
    ConsensusParms const & parms = adaptor_.parms();

    auto const peerCutoff = now_ - parms.proposeFRESHNESS;
    auto const ourCutoff = now_ - parms.proposeINTERVAL;

    std::map<NetClock::time_point, int> closeTimeVotes;
    {
        auto it = currPeerPositions_.begin();
        while (it != currPeerPositions_.end())
        {
            Proposal_t const& peerProp = it->second.proposal();
            if (peerProp.isStale(peerCutoff))
            {
                NodeID_t const& peerID = peerProp.nodeID();
                JLOG(j_.warn()) << "Removing stale proposal from " << peerID;
                for (auto& dt : result_->disputes)
                    dt.second.unVote(peerID);
                it = currPeerPositions_.erase(it);
            }
            else
            {
                ++closeTimeVotes[asCloseTime(peerProp.closeTime())];
                ++it;
            }
        }
    }

    boost::optional<TxSet_t> ourNewSet;

    {
        boost::optional<typename TxSet_t::MutableTxSet> mutableSet;
        for (auto& it : result_->disputes)
        {
            if (it.second.updateVote(
                    convergePercent_,
                    mode_.get()== ConsensusMode::proposing,
                    parms))
            {
                if (!mutableSet)
                    mutableSet.emplace(result_->txns);

                if (it.second.getOurVote())
                {
                    mutableSet->insert(it.second.tx());
                }
                else
                {
                    mutableSet->erase(it.first);
                }
            }
        }

        if (mutableSet)
            ourNewSet.emplace(std::move(*mutableSet));
    }

    NetClock::time_point consensusCloseTime = {};
    haveCloseTimeConsensus_ = false;

    if (currPeerPositions_.empty())
    {
        haveCloseTimeConsensus_ = true;
        consensusCloseTime = asCloseTime(result_->position.closeTime());
    }
    else
    {
        int neededWeight;

        if (convergePercent_ < parms.avMID_CONSENSUS_TIME)
            neededWeight = parms.avINIT_CONSENSUS_PCT;
        else if (convergePercent_ < parms.avLATE_CONSENSUS_TIME)
            neededWeight = parms.avMID_CONSENSUS_PCT;
        else if (convergePercent_ < parms.avSTUCK_CONSENSUS_TIME)
            neededWeight = parms.avLATE_CONSENSUS_PCT;
        else
            neededWeight = parms.avSTUCK_CONSENSUS_PCT;

        int participants = currPeerPositions_.size();
        if (mode_.get() == ConsensusMode::proposing)
        {
            ++closeTimeVotes[asCloseTime(result_->position.closeTime())];
            ++participants;
        }

        int threshVote = participantsNeeded(participants, neededWeight);

        int const threshConsensus =
            participantsNeeded(participants, parms.avCT_CONSENSUS_PCT);

        JLOG(j_.info()) << "Proposers:" << currPeerPositions_.size()
                        << " nw:" << neededWeight << " thrV:" << threshVote
                        << " thrC:" << threshConsensus;

        for (auto const& it : closeTimeVotes)
        {
            JLOG(j_.debug())
                << "CCTime: seq "
                << static_cast<std::uint32_t>(previousLedger_.seq()) + 1 << ": "
                << it.first.time_since_epoch().count() << " has " << it.second
                << ", " << threshVote << " required";

            if (it.second >= threshVote)
            {
                consensusCloseTime = it.first;
                threshVote = it.second;

                if (threshVote >= threshConsensus)
                    haveCloseTimeConsensus_ = true;
            }
        }

        if (!haveCloseTimeConsensus_)
        {
            JLOG(j_.debug())
                << "No CT consensus:"
                << " Proposers:" << currPeerPositions_.size()
                << " Mode:" << to_string(mode_.get())
                << " Thresh:" << threshConsensus
                << " Pos:" << consensusCloseTime.time_since_epoch().count();
        }
    }

    if (!ourNewSet &&
        ((consensusCloseTime != asCloseTime(result_->position.closeTime())) ||
         result_->position.isStale(ourCutoff)))
    {
        ourNewSet.emplace(result_->txns);
    }

    if (ourNewSet)
    {
        auto newID = ourNewSet->id();

        result_->txns = std::move(*ourNewSet);

        JLOG(j_.info()) << "Position change: CTime "
                        << consensusCloseTime.time_since_epoch().count()
                        << ", tx " << newID;

        result_->position.changePosition(newID, consensusCloseTime, now_);

        if (acquired_.emplace(newID, result_->txns).second)
        {
            if (!result_->position.isBowOut())
                adaptor_.share(result_->txns);

            for (auto const& it : currPeerPositions_)
            {
                Proposal_t const& p = it.second.proposal();
                if (p.position() == newID)
                    updateDisputes(it.first, result_->txns);
            }
        }

        if (!result_->position.isBowOut() &&
            (mode_.get() == ConsensusMode::proposing))
            adaptor_.propose(result_->position);
    }
}

template <class Adaptor>
bool
Consensus<Adaptor>::haveConsensus()
{
    assert(result_);

    int agree = 0, disagree = 0;

    auto ourPosition = result_->position.position();

    for (auto const& it : currPeerPositions_)
    {
        Proposal_t const& peerProp = it.second.proposal();
        if (peerProp.position() == ourPosition)
        {
            ++agree;
        }
        else
        {
            using std::to_string;

            JLOG(j_.debug()) << to_string(it.first) << " has "
                             << to_string(peerProp.position());
            ++disagree;
        }
    }
    auto currentFinished =
        adaptor_.proposersFinished(previousLedger_, prevLedgerID_);

    JLOG(j_.debug()) << "Checking for TX consensus: agree=" << agree
                     << ", disagree=" << disagree;

    result_->state = checkConsensus(
        prevProposers_,
        agree + disagree,
        agree,
        currentFinished,
        prevRoundTime_,
        result_->roundTime.read(),
        adaptor_.parms(),
        mode_.get() == ConsensusMode::proposing,
        j_);

    if (result_->state == ConsensusState::No)
        return false;

    if (result_->state == ConsensusState::MovedOn)
    {
        JLOG(j_.error()) << "Unable to reach consensus";
        JLOG(j_.error()) << Json::Compact{getJson(true)};
    }

    return true;
}

template <class Adaptor>
void
Consensus<Adaptor>::leaveConsensus()
{
    if (mode_.get() == ConsensusMode::proposing)
    {
        if (result_ && !result_->position.isBowOut())
        {
            result_->position.bowOut(now_);
            adaptor_.propose(result_->position);
        }

        mode_.set(ConsensusMode::observing, adaptor_);
        JLOG(j_.info()) << "Bowing out of consensus";
    }
}

template <class Adaptor>
void
Consensus<Adaptor>::createDisputes(TxSet_t const& o)
{
    assert(result_);

    if (!result_->compares.emplace(o.id()).second)
        return;

    if (result_->txns.id() == o.id())
        return;

    JLOG(j_.debug()) << "createDisputes " << result_->txns.id() << " to "
                     << o.id();

    auto differences = result_->txns.compare(o);

    int dc = 0;

    for (auto& id : differences)
    {
        ++dc;
        assert(
            (id.second && result_->txns.find(id.first) && !o.find(id.first)) ||
            (!id.second && !result_->txns.find(id.first) && o.find(id.first)));

        Tx_t tx = id.second ? *result_->txns.find(id.first) : *o.find(id.first);
        auto txID = tx.id();

        if (result_->disputes.find(txID) != result_->disputes.end())
            continue;

        JLOG(j_.debug()) << "Transaction " << txID << " is disputed";

        typename Result::Dispute_t dtx{tx, result_->txns.exists(txID),
         std::max(prevProposers_, currPeerPositions_.size()), j_};

        for (auto const& pit : currPeerPositions_)
        {
            Proposal_t const& peerProp = pit.second.proposal();
            auto const cit = acquired_.find(peerProp.position());
            if (cit != acquired_.end())
                dtx.setVote(pit.first, cit->second.exists(txID));
        }
        adaptor_.share(dtx.tx());

        result_->disputes.emplace(txID, std::move(dtx));
    }
    JLOG(j_.debug()) << dc << " differences found";
}

template <class Adaptor>
void
Consensus<Adaptor>::updateDisputes(NodeID_t const& node, TxSet_t const& other)
{
    assert(result_);

    if (result_->compares.find(other.id()) == result_->compares.end())
        createDisputes(other);

    for (auto& it : result_->disputes)
    {
        auto& d = it.second;
        d.setVote(node, other.exists(d.tx().id()));
    }
}

template <class Adaptor>
NetClock::time_point
Consensus<Adaptor>::asCloseTime(NetClock::time_point raw) const
{
    if (adaptor_.parms().useRoundedCloseTime)
        return roundCloseTime(raw, closeResolution_);
    else
        return effCloseTime(raw, closeResolution_, previousLedger_.closeTime());
}

}  

#endif








