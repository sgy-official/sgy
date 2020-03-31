

#ifndef RIPPLE_TEST_CSF_UNL_H_INCLUDED
#define RIPPLE_TEST_CSF_UNL_H_INCLUDED

#include <test/csf/random.h>
#include <boost/container/flat_set.hpp>
#include <boost/optional.hpp>
#include <chrono>
#include <numeric>
#include <random>
#include <vector>

namespace ripple {
namespace test {
namespace csf {


template <class Peer>
class TrustGraph
{
    using Graph = Digraph<Peer>;

    Graph graph_;

public:
    
    TrustGraph() = default;

    Graph const&
    graph()
    {
        return graph_;
    }

    
    void
    trust(Peer const& from, Peer const& to)
    {
        graph_.connect(from, to);
    }

    
    void
    untrust(Peer const& from, Peer const& to)
    {
        graph_.disconnect(from, to);
    }

    bool
    trusts(Peer const& from, Peer const& to) const
    {
        return graph_.connected(from, to);
    }

    
    auto
    trustedPeers(Peer const & a) const
    {
        return graph_.outVertices(a);
    }

    
    struct ForkInfo
    {
        std::set<Peer> unlA;
        std::set<Peer> unlB;
        int overlap;
        double required;
    };

    std::vector<ForkInfo>
    forkablePairs(double quorum) const
    {


        using UNL = std::set<Peer>;
        std::set<UNL> unique;
        for (Peer const & peer : graph_.outVertices())
        {
            unique.emplace(
                std::begin(trustedPeers(peer)), std::end(trustedPeers(peer)));
        }

        std::vector<UNL> uniqueUNLs(unique.begin(), unique.end());
        std::vector<ForkInfo> res;

        for (int i = 0; i < uniqueUNLs.size(); ++i)
        {
            for (int j = (i + 1); j < uniqueUNLs.size(); ++j)
            {
                auto const& unlA = uniqueUNLs[i];
                auto const& unlB = uniqueUNLs[j];
                double rhs =
                    2.0 * (1. - quorum) * std::max(unlA.size(), unlB.size());

                int intersectionSize = std::count_if(
                    unlA.begin(), unlA.end(), [&](Peer p) {
                        return unlB.find(p) != unlB.end();
                    });

                if (intersectionSize < rhs)
                {
                    res.emplace_back(ForkInfo{unlA, unlB, intersectionSize, rhs});
                }
            }
        }
        return res;
    }

    
    bool
    canFork(double quorum) const
    {
        return !forkablePairs(quorum).empty();
    }
};

}  
}  
}  

#endif








