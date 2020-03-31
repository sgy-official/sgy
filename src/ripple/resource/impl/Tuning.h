

#ifndef RIPPLE_RESOURCE_TUNING_H_INCLUDED
#define RIPPLE_RESOURCE_TUNING_H_INCLUDED

#include <chrono>

namespace ripple {
namespace Resource {


enum
{
     warningThreshold           = 5000

    ,dropThreshold              = 15000

    ,decayWindowSeconds         = 32

    ,minimumGossipBalance       = 1000
};

std::chrono::seconds constexpr secondsUntilExpiration{300};

std::chrono::seconds constexpr gossipExpirationSeconds{30};

}
}

#endif








