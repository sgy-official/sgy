

#ifndef RIPPLE_NODESTORE_TUNING_H_INCLUDED
#define RIPPLE_NODESTORE_TUNING_H_INCLUDED

namespace ripple {
namespace NodeStore {

enum
{
    cacheTargetSize     = 16384

    ,asyncDivider = 8
};

std::chrono::seconds constexpr cacheTargetAge = std::chrono::minutes{5};
auto constexpr shardCacheSz = 16384;
std::chrono::seconds constexpr shardCacheAge = std::chrono::minutes{1};

}
}

#endif








