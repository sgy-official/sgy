

#ifndef RIPPLE_APP_CONSENSUS_RCLCONSENSUS_H_INCLUDED
#define RIPPLE_APP_CONSENSUS_RCLCONSENSUS_H_INCLUDED

#include <ripple/app/consensus/RCLCxLedger.h>
#include <ripple/app/consensus/RCLCxPeerPos.h>
#include <ripple/app/consensus/RCLCxTx.h>
#include <ripple/app/consensus/RCLCensorshipDetector.h>
#include <ripple/app/misc/FeeVote.h>
#include <ripple/basics/CountedObject.h>
#include <ripple/basics/Log.h>
#include <ripple/beast/utility/Journal.h>
#include <ripple/consensus/Consensus.h>
#include <ripple/core/JobQueue.h>
#include <ripple/overlay/Message.h>
#include <ripple/protocol/RippleLedgerHash.h>
#include <ripple/protocol/STValidation.h>
#include <ripple/shamap/SHAMap.h>
#include <atomic>
#include <mutex>
#include <set>
namespace ripple {

class InboundTransactions;
class LocalTxs;
class LedgerMaster;
class ValidatorKeys;


class RCLConsensus
{
    
    constexpr static unsigned int censorshipWarnInternal = 15;

    class Adaptor
    {
        Application& app_;
        std::unique_ptr<FeeVote> feeVote_;
        LedgerMaster& ledgerMaster_;
        LocalTxs& localTxs_;
        InboundTransactions& inboundTransactions_;
        beast::Journal j_;

        NodeID const nodeID_;
        PublicKey const valPublic_;
        SecretKey const valSecret_;

        LedgerHash acquiringLedger_;
        ConsensusParms parms_;

        NetClock::time_point lastValidationTime_;

        std::atomic<bool> validating_{false};
        std::atomic<std::size_t> prevProposers_{0};
        std::atomic<std::chrono::milliseconds> prevRoundTime_{
            std::chrono::milliseconds{0}};
        std::atomic<ConsensusMode> mode_{ConsensusMode::observing};

        RCLCensorshipDetector<TxID, LedgerIndex> censorshipDetector_;

    public:
        using Ledger_t = RCLCxLedger;
        using NodeID_t = NodeID;
        using NodeKey_t = PublicKey;
        using TxSet_t = RCLTxSet;
        using PeerPosition_t = RCLCxPeerPos;

        using Result = ConsensusResult<Adaptor>;

        Adaptor(
            Application& app,
            std::unique_ptr<FeeVote>&& feeVote,
            LedgerMaster& ledgerMaster,
            LocalTxs& localTxs,
            InboundTransactions& inboundTransactions,
            ValidatorKeys const & validatorKeys,
            beast::Journal journal);

        bool
        validating() const
        {
            return validating_;
        }

        std::size_t
        prevProposers() const
        {
            return prevProposers_;
        }

        std::chrono::milliseconds
        prevRoundTime() const
        {
            return prevRoundTime_;
        }

        ConsensusMode
        mode() const
        {
            return mode_;
        }

        
        bool
        preStartRound(RCLCxLedger const & prevLedger);

        bool
        haveValidated() const;

        LedgerIndex
        getValidLedgerIndex() const;

        std::pair<std::size_t, hash_set<NodeKey_t>>
        getQuorumKeys() const;

        std::size_t
        laggards(Ledger_t::Seq const seq,
            hash_set<NodeKey_t >& trustedKeys) const;

        
        bool
        validator() const;

        
        ConsensusParms const&
        parms() const
        {
            return parms_;
        }

    private:
        friend class Consensus<Adaptor>;

        
        boost::optional<RCLCxLedger>
        acquireLedger(LedgerHash const& hash);

        
        void
        share(RCLCxPeerPos const& peerPos);

        
        void
        share(RCLCxTx const& tx);

        
        boost::optional<RCLTxSet>
        acquireTxSet(RCLTxSet::ID const& setId);

        
        bool
        hasOpenTransactions() const;

        
        std::size_t
        proposersValidated(LedgerHash const& h) const;

        
        std::size_t
        proposersFinished(RCLCxLedger const & ledger, LedgerHash const& h) const;

        
        void
        propose(RCLCxPeerPos::Proposal const& proposal);

        
        void
        share(RCLTxSet const& txns);

        
        uint256
        getPrevLedger(
            uint256 ledgerID,
            RCLCxLedger const& ledger,
            ConsensusMode mode);

        
        void
        onModeChange(
            ConsensusMode before,
            ConsensusMode after);

        
        Result
        onClose(
            RCLCxLedger const& ledger,
            NetClock::time_point const& closeTime,
            ConsensusMode mode);

        
        void
        onAccept(
            Result const& result,
            RCLCxLedger const& prevLedger,
            NetClock::duration const& closeResolution,
            ConsensusCloseTimes const& rawCloseTimes,
            ConsensusMode const& mode,
            Json::Value&& consensusJson);

        
        void
        onForceAccept(
            Result const& result,
            RCLCxLedger const& prevLedger,
            NetClock::duration const& closeResolution,
            ConsensusCloseTimes const& rawCloseTimes,
            ConsensusMode const& mode,
            Json::Value&& consensusJson);

        
        void
        notify(
            protocol::NodeEvent ne,
            RCLCxLedger const& ledger,
            bool haveCorrectLCL);

        
        void
        doAccept(
            Result const& result,
            RCLCxLedger const& prevLedger,
            NetClock::duration closeResolution,
            ConsensusCloseTimes const& rawCloseTimes,
            ConsensusMode const& mode,
            Json::Value&& consensusJson);

        
        RCLCxLedger
        buildLCL(
            RCLCxLedger const& previousLedger,
            CanonicalTXSet& retriableTxs,
            NetClock::time_point closeTime,
            bool closeTimeCorrect,
            NetClock::duration closeResolution,
            std::chrono::milliseconds roundTime,
            std::set<TxID>& failedTxs);

        
        void
        validate(RCLCxLedger const& ledger, RCLTxSet const& txns, bool proposing);
    };

public:
    RCLConsensus(
        Application& app,
        std::unique_ptr<FeeVote>&& feeVote,
        LedgerMaster& ledgerMaster,
        LocalTxs& localTxs,
        InboundTransactions& inboundTransactions,
        Consensus<Adaptor>::clock_type const& clock,
        ValidatorKeys const & validatorKeys,
        beast::Journal journal);

    RCLConsensus(RCLConsensus const&) = delete;

    RCLConsensus&
    operator=(RCLConsensus const&) = delete;

    bool
    validating() const
    {
        return adaptor_.validating();
    }

    std::size_t
    prevProposers() const
    {
        return adaptor_.prevProposers();
    }

    
    std::chrono::milliseconds
    prevRoundTime() const
    {
        return adaptor_.prevRoundTime();
    }

    ConsensusMode
    mode() const
    {
        return adaptor_.mode();
    }

    Json::Value
    getJson(bool full) const;

    void
    startRound(
        NetClock::time_point const& now,
        RCLCxLedger::ID const& prevLgrId,
        RCLCxLedger const& prevLgr,
        hash_set<NodeID> const& nowUntrusted);

    void
    timerEntry(NetClock::time_point const& now);

    void
    gotTxSet(NetClock::time_point const& now, RCLTxSet const& txSet);

    RCLCxLedger::ID
    prevLedgerID() const
    {
        ScopedLockType _{mutex_};
        return consensus_.prevLedgerID();
    }

    void
    simulate(
        NetClock::time_point const& now,
        boost::optional<std::chrono::milliseconds> consensusDelay);

    bool
    peerProposal(
        NetClock::time_point const& now,
        RCLCxPeerPos const& newProposal);

    ConsensusParms const &
    parms() const
    {
        return adaptor_.parms();
    }

private:
    mutable std::recursive_mutex mutex_;
    using ScopedLockType = std::lock_guard <std::recursive_mutex>;

    Adaptor adaptor_;
    Consensus<Adaptor> consensus_;
    beast::Journal j_;
};
}

#endif








