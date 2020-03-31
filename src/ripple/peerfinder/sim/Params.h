

#ifndef RIPPLE_PEERFINDER_SIM_PARAMS_H_INCLUDED
#define RIPPLE_PEERFINDER_SIM_PARAMS_H_INCLUDED

namespace ripple {
namespace PeerFinder {
namespace Sim {


struct Params
{
    Params ()
        : steps (50)
        , nodes (10)
        , maxPeers (20)
        , outPeers (9.5)
        , firewalled (0)
    {
    }

    int steps;
    int nodes;
    int maxPeers;
    double outPeers;
    double firewalled; 
};

}
}
}

#endif








