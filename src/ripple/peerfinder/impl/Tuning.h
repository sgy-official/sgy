

#ifndef RIPPLE_PEERFINDER_TUNING_H_INCLUDED
#define RIPPLE_PEERFINDER_TUNING_H_INCLUDED

#include <array>

namespace ripple {
namespace PeerFinder {



namespace Tuning {

enum
{

    
    secondsPerConnect = 10

    
    ,maxConnectAttempts = 20

    
    ,outPercent = 15

    
    ,minOutCount = 10

    
    ,defaultMaxPeers = 21

    
    ,maxRedirects = 30
};


static std::array <int, 10> const connectionBackoff
    {{ 1, 1, 2, 3, 5, 8, 13, 21, 34, 55 }};


enum
{
    bootcacheSize = 1000

    ,bootcachePrunePercent = 10
};

static std::chrono::seconds const bootcacheCooldownTime (60);


enum
{
    maxHops = 6

    ,numberOfEndpoints = 2 * maxHops

    ,numberOfEndpointsMax = 20

    ,defaultMaxPeerCount = 21

    
    ,redirectEndpointCount = 10
};

static std::chrono::seconds const secondsPerMessage (5);

static std::chrono::seconds const liveCacheSecondsToLive (30);


static std::chrono::seconds const recentAttemptDuration (60);

}


}
}

#endif








