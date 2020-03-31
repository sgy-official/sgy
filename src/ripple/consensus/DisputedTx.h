

#ifndef RIPPLE_APP_CONSENSUS_IMPL_DISPUTEDTX_H_INCLUDED
#define RIPPLE_APP_CONSENSUS_IMPL_DISPUTEDTX_H_INCLUDED

#include <boost/container/flat_map.hpp>
#include <ripple/basics/Log.h>
#include <ripple/basics/base_uint.h>
#include <ripple/beast/utility/Journal.h>
#include <ripple/consensus/ConsensusParms.h>
#include <ripple/json/json_writer.h>
#include <ripple/protocol/Serializer.h>
#include <ripple/protocol/UintTypes.h>
#include <memory>

namespace ripple {



template <class Tx_t, class NodeID_t>
class DisputedTx
{
    using TxID_t = typename Tx_t::ID;
    using Map_t = boost::container::flat_map<NodeID_t, bool>;
public:
    
    DisputedTx(Tx_t const& tx, bool ourVote, std::size_t numPeers, beast::Journal j)
        : yays_(0), nays_(0), ourVote_(ourVote), tx_(tx), j_(j)
    {
        votes_.reserve(numPeers);
    }

    TxID_t const&
    ID() const
    {
        return tx_.id();
    }

    bool
    getOurVote() const
    {
        return ourVote_;
    }

    Tx_t const&
    tx() const
    {
        return tx_;
    }

    void
    setOurVote(bool o)
    {
        ourVote_ = o;
    }

    
    void
    setVote(NodeID_t const& peer, bool votesYes);

    
    void
    unVote(NodeID_t const& peer);

    
    bool
    updateVote(int percentTime, bool proposing, ConsensusParms const& p);

    Json::Value
    getJson() const;

private:
    int yays_;      
    int nays_;      
    bool ourVote_;  
    Tx_t tx_;       
    Map_t votes_;   
    beast::Journal j_;
};

template <class Tx_t, class NodeID_t>
void
DisputedTx<Tx_t, NodeID_t>::setVote(NodeID_t const& peer, bool votesYes)
{
    auto res = votes_.insert(std::make_pair(peer, votesYes));

    if (res.second)
    {
        if (votesYes)
        {
            JLOG(j_.debug()) << "Peer " << peer << " votes YES on " << tx_.id();
            ++yays_;
        }
        else
        {
            JLOG(j_.debug()) << "Peer " << peer << " votes NO on " << tx_.id();
            ++nays_;
        }
    }
    else if (votesYes && !res.first->second)
    {
        JLOG(j_.debug()) << "Peer " << peer << " now votes YES on " << tx_.id();
        --nays_;
        ++yays_;
        res.first->second = true;
    }
    else if (!votesYes && res.first->second)
    {
        JLOG(j_.debug()) << "Peer " << peer << " now votes NO on " << tx_.id();
        ++nays_;
        --yays_;
        res.first->second = false;
    }
}

template <class Tx_t, class NodeID_t>
void
DisputedTx<Tx_t, NodeID_t>::unVote(NodeID_t const& peer)
{
    auto it = votes_.find(peer);

    if (it != votes_.end())
    {
        if (it->second)
            --yays_;
        else
            --nays_;

        votes_.erase(it);
    }
}

template <class Tx_t, class NodeID_t>
bool
DisputedTx<Tx_t, NodeID_t>::updateVote(
    int percentTime,
    bool proposing,
    ConsensusParms const& p)
{
    if (ourVote_ && (nays_ == 0))
        return false;

    if (!ourVote_ && (yays_ == 0))
        return false;

    bool newPosition;
    int weight;

    if (proposing)  
    {
        weight = (yays_ * 100 + (ourVote_ ? 100 : 0)) / (nays_ + yays_ + 1);

        if (percentTime < p.avMID_CONSENSUS_TIME)
            newPosition = weight > p.avINIT_CONSENSUS_PCT;
        else if (percentTime < p.avLATE_CONSENSUS_TIME)
            newPosition = weight > p.avMID_CONSENSUS_PCT;
        else if (percentTime < p.avSTUCK_CONSENSUS_TIME)
            newPosition = weight > p.avLATE_CONSENSUS_PCT;
        else
            newPosition = weight > p.avSTUCK_CONSENSUS_PCT;
    }
    else
    {
        weight = -1;
        newPosition = yays_ > nays_;
    }

    if (newPosition == ourVote_)
    {
        JLOG(j_.info()) << "No change (" << (ourVote_ ? "YES" : "NO")
                        << ") : weight " << weight << ", percent "
                        << percentTime;
        JLOG(j_.debug()) << Json::Compact{getJson()};
        return false;
    }

    ourVote_ = newPosition;
    JLOG(j_.debug()) << "We now vote " << (ourVote_ ? "YES" : "NO") << " on "
                     << tx_.id();
    JLOG(j_.debug()) << Json::Compact{getJson()};
    return true;
}

template <class Tx_t, class NodeID_t>
Json::Value
DisputedTx<Tx_t, NodeID_t>::getJson() const
{
    using std::to_string;

    Json::Value ret(Json::objectValue);

    ret["yays"] = yays_;
    ret["nays"] = nays_;
    ret["our_vote"] = ourVote_;

    if (!votes_.empty())
    {
        Json::Value votesj(Json::objectValue);
        for (auto& vote : votes_)
            votesj[to_string(vote.first)] = vote.second;
        ret["votes"] = std::move(votesj);
    }

    return ret;
}

}  

#endif








