

#ifndef RIPPLE_CONSENSUS_CONSENSUS_TYPES_H_INCLUDED
#define RIPPLE_CONSENSUS_CONSENSUS_TYPES_H_INCLUDED

#include <ripple/basics/chrono.h>
#include <ripple/consensus/ConsensusProposal.h>
#include <ripple/consensus/DisputedTx.h>
#include <chrono>
#include <map>

namespace ripple {


enum class ConsensusMode {
    proposing,
    observing,
    wrongLedger,
    switchedLedger
};

inline std::string
to_string(ConsensusMode m)
{
    switch (m)
    {
        case ConsensusMode::proposing:
            return "proposing";
        case ConsensusMode::observing:
            return "observing";
        case ConsensusMode::wrongLedger:
            return "wrongLedger";
        case ConsensusMode::switchedLedger:
            return "switchedLedger";
        default:
            return "unknown";
    }
}


enum class ConsensusPhase {
    open,

    establish,

    accepted,
};

inline std::string
to_string(ConsensusPhase p)
{
    switch (p)
    {
        case ConsensusPhase::open:
            return "open";
        case ConsensusPhase::establish:
            return "establish";
        case ConsensusPhase::accepted:
            return "accepted";
        default:
            return "unknown";
    }
}


class ConsensusTimer
{
    using time_point = std::chrono::steady_clock::time_point;
    time_point start_;
    std::chrono::milliseconds dur_;

public:
    std::chrono::milliseconds
    read() const
    {
        return dur_;
    }

    void
    tick(std::chrono::milliseconds fixed)
    {
        dur_ += fixed;
    }

    void
    reset(time_point tp)
    {
        start_ = tp;
        dur_ = std::chrono::milliseconds{0};
    }

    void
    tick(time_point tp)
    {
        using namespace std::chrono;
        dur_ = duration_cast<milliseconds>(tp - start_);
    }
};


struct ConsensusCloseTimes
{
    explicit ConsensusCloseTimes() = default;

    std::map<NetClock::time_point, int> peers;

    NetClock::time_point self;
};


enum class ConsensusState {
    No,       
    MovedOn,  
    Yes       
};


template <class Traits>
struct ConsensusResult
{
    using Ledger_t = typename Traits::Ledger_t;
    using TxSet_t = typename Traits::TxSet_t;
    using NodeID_t = typename Traits::NodeID_t;

    using Tx_t = typename TxSet_t::Tx;
    using Proposal_t = ConsensusProposal<
        NodeID_t,
        typename Ledger_t::ID,
        typename TxSet_t::ID>;
    using Dispute_t = DisputedTx<Tx_t, NodeID_t>;

    ConsensusResult(TxSet_t&& s, Proposal_t&& p)
        : txns{std::move(s)}, position{std::move(p)}
    {
        assert(txns.id() == position.position());
    }

    TxSet_t txns;

    Proposal_t position;

    hash_map<typename Tx_t::ID, Dispute_t> disputes;

    hash_set<typename TxSet_t::ID> compares;

    ConsensusTimer roundTime;

    ConsensusState state = ConsensusState::No;

    std::size_t proposers = 0;
};
}  

#endif








