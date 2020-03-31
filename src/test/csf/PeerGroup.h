
#ifndef RIPPLE_TEST_CSF_PEERGROUP_H_INCLUDED
#define RIPPLE_TEST_CSF_PEERGROUP_H_INCLUDED

#include <algorithm>
#include <test/csf/Peer.h>
#include <test/csf/random.h>
#include <vector>

namespace ripple {
namespace test {
namespace csf {


class PeerGroup
{
    using peers_type = std::vector<Peer*>;
    peers_type peers_;
public:
    using iterator = peers_type::iterator;
    using const_iterator = peers_type::const_iterator;
    using reference = peers_type::reference;
    using const_reference = peers_type::const_reference;

    PeerGroup() = default;
    PeerGroup(Peer* peer) : peers_{1, peer}
    {
    }
    PeerGroup(std::vector<Peer*>&& peers) : peers_{std::move(peers)}
    {
        std::sort(peers_.begin(), peers_.end());
    }
    PeerGroup(std::vector<Peer*> const& peers) : peers_{peers}
    {
        std::sort(peers_.begin(), peers_.end());
    }

    PeerGroup(std::set<Peer*> const& peers) : peers_{peers.begin(), peers.end()}
    {
    }

    iterator
    begin()
    {
        return peers_.begin();
    }

    iterator
    end()
    {
        return peers_.end();
    }

    const_iterator
    begin() const
    {
        return peers_.begin();
    }

    const_iterator
    end() const
    {
        return peers_.end();
    }

    const_reference
    operator[](std::size_t i) const
    {
        return peers_[i];
    }

    bool
    contains(Peer const * p)
    {
        return std::find(peers_.begin(), peers_.end(), p) != peers_.end();
    }

    bool
    contains(PeerID id)
    {
        return std::find_if(peers_.begin(), peers_.end(), [id](Peer const* p) {
                   return p->id == id;
               }) != peers_.end();
    }

    std::size_t
    size() const
    {
        return peers_.size();
    }

    
    void
    trust(PeerGroup const & o)
    {
        for(Peer * p : peers_)
        {
            for (Peer * target : o.peers_)
            {
                p->trust(*target);
            }
        }
    }

    
    void
    untrust(PeerGroup const & o)
    {
        for(Peer * p : peers_)
        {
            for (Peer * target : o.peers_)
            {
                p->untrust(*target);
            }
        }
    }

    
    void
    connect(PeerGroup const& o, SimDuration delay)
    {
        for(Peer * p : peers_)
        {
            for (Peer * target : o.peers_)
            {
                if(p != target)
                    p->connect(*target, delay);
            }
        }
    }

    
    void
    disconnect(PeerGroup const &o)
    {
        for(Peer * p : peers_)
        {
            for (Peer * target : o.peers_)
            {
                p->disconnect(*target);
            }
        }
    }

    
    void
    trustAndConnect(PeerGroup const & o, SimDuration delay)
    {
        trust(o);
        connect(o, delay);
    }

    
    void
    connectFromTrust(SimDuration delay)
    {
        for (Peer * peer : peers_)
        {
            for (Peer * to : peer->trustGraph.trustedPeers(peer))
            {
                peer->connect(*to, delay);
            }
        }
    }

    friend
    PeerGroup
    operator+(PeerGroup const & a, PeerGroup const & b)
    {
        PeerGroup res;
        std::set_union(
            a.peers_.begin(),
            a.peers_.end(),
            b.peers_.begin(),
            b.peers_.end(),
            std::back_inserter(res.peers_));
        return res;
    }

    friend
    PeerGroup
    operator-(PeerGroup const & a, PeerGroup const & b)
    {
        PeerGroup res;

        std::set_difference(
            a.peers_.begin(),
            a.peers_.end(),
            b.peers_.begin(),
            b.peers_.end(),
            std::back_inserter(res.peers_));

        return res;
    }

    friend std::ostream&
    operator<<(std::ostream& o, PeerGroup const& t)
    {
        o << "{";
        bool first = true;
        for (Peer const* p : t)
        {
            if(!first)
                o << ", ";
            first = false;
            o << p->id;
        }
        o << "}";
        return o;
    }
};


template <class RandomNumberDistribution, class Generator>
std::vector<PeerGroup>
randomRankedGroups(
    PeerGroup & peers,
    std::vector<double> const & ranks,
    int numGroups,
    RandomNumberDistribution sizeDist,
    Generator& g)
{
    assert(peers.size() == ranks.size());

    std::vector<PeerGroup> groups;
    groups.reserve(numGroups);
    std::vector<Peer*> rawPeers(peers.begin(), peers.end());
    std::generate_n(std::back_inserter(groups), numGroups, [&]() {
        std::vector<Peer*> res = random_weighted_shuffle(rawPeers, ranks, g);
        res.resize(sizeDist(g));
        return PeerGroup(std::move(res));
    });

    return groups;
}




template <class RandomNumberDistribution, class Generator>
void
randomRankedTrust(
    PeerGroup & peers,
    std::vector<double> const & ranks,
    int numGroups,
    RandomNumberDistribution sizeDist,
    Generator& g)
{
    std::vector<PeerGroup> const groups =
        randomRankedGroups(peers, ranks, numGroups, sizeDist, g);

    std::uniform_int_distribution<int> u(0, groups.size() - 1);
    for(auto & peer : peers)
    {
        for(auto & target : groups[u(g)])
             peer->trust(*target);
    }
}


template <class RandomNumberDistribution, class Generator>
void
randomRankedConnect(
    PeerGroup & peers,
    std::vector<double> const & ranks,
    int numGroups,
    RandomNumberDistribution sizeDist,
    Generator& g,
    SimDuration delay)
{
    std::vector<PeerGroup> const groups =
        randomRankedGroups(peers, ranks, numGroups, sizeDist, g);

    std::uniform_int_distribution<int> u(0, groups.size() - 1);
    for(auto & peer : peers)
    {
        for(auto & target : groups[u(g)])
             peer->connect(*target, delay);
    }
}

}  
}  
}  
#endif









