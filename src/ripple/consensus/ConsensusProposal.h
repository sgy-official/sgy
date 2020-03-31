
#ifndef RIPPLE_CONSENSUS_ConsensusProposal_H_INCLUDED
#define RIPPLE_CONSENSUS_ConsensusProposal_H_INCLUDED

#include <ripple/basics/chrono.h>
#include <ripple/json/json_value.h>
#include <ripple/protocol/jss.h>
#include <cstdint>

namespace ripple {

template <class NodeID_t, class LedgerID_t, class Position_t>
class ConsensusProposal
{
public:
    using NodeID = NodeID_t;

    static std::uint32_t const seqJoin = 0;

    static std::uint32_t const seqLeave = 0xffffffff;

    
    ConsensusProposal(
        LedgerID_t const& prevLedger,
        std::uint32_t seq,
        Position_t const& position,
        NetClock::time_point closeTime,
        NetClock::time_point now,
        NodeID_t const& nodeID)
        : previousLedger_(prevLedger)
        , position_(position)
        , closeTime_(closeTime)
        , time_(now)
        , proposeSeq_(seq)
        , nodeID_(nodeID)
    {
    }

    NodeID_t const&
    nodeID() const
    {
        return nodeID_;
    }

    Position_t const&
    position() const
    {
        return position_;
    }

    LedgerID_t const&
    prevLedger() const
    {
        return previousLedger_;
    }

    
    std::uint32_t
    proposeSeq() const
    {
        return proposeSeq_;
    }

    NetClock::time_point const&
    closeTime() const
    {
        return closeTime_;
    }

    NetClock::time_point const&
    seenTime() const
    {
        return time_;
    }

    
    bool
    isInitial() const
    {
        return proposeSeq_ == seqJoin;
    }

    bool
    isBowOut() const
    {
        return proposeSeq_ == seqLeave;
    }

    bool
    isStale(NetClock::time_point cutoff) const
    {
        return time_ <= cutoff;
    }

    
    void
    changePosition(
        Position_t const& newPosition,
        NetClock::time_point newCloseTime,
        NetClock::time_point now)
    {
        position_ = newPosition;
        closeTime_ = newCloseTime;
        time_ = now;
        if (proposeSeq_ != seqLeave)
            ++proposeSeq_;
    }

    
    void
    bowOut(NetClock::time_point now)
    {
        time_ = now;
        proposeSeq_ = seqLeave;
    }

    Json::Value
    getJson() const
    {
        using std::to_string;

        Json::Value ret = Json::objectValue;
        ret[jss::previous_ledger] = to_string(prevLedger());

        if (!isBowOut())
        {
            ret[jss::transaction_hash] = to_string(position());
            ret[jss::propose_seq] = proposeSeq();
        }

        ret[jss::close_time] =
            to_string(closeTime().time_since_epoch().count());

        return ret;
    }

private:
    LedgerID_t previousLedger_;

    Position_t position_;

    NetClock::time_point closeTime_;

    NetClock::time_point time_;

    std::uint32_t proposeSeq_;

    NodeID_t nodeID_;
};

template <class NodeID_t, class LedgerID_t, class Position_t>
bool
operator==(
    ConsensusProposal<NodeID_t, LedgerID_t, Position_t> const& a,
    ConsensusProposal<NodeID_t, LedgerID_t, Position_t> const& b)
{
    return a.nodeID() == b.nodeID() && a.proposeSeq() == b.proposeSeq() &&
        a.prevLedger() == b.prevLedger() && a.position() == b.position() &&
        a.closeTime() == b.closeTime() && a.seenTime() == b.seenTime();
}
}
#endif








