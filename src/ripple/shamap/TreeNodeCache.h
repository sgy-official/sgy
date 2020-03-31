

#ifndef RIPPLE_SHAMAP_TREENODECACHE_H_INCLUDED
#define RIPPLE_SHAMAP_TREENODECACHE_H_INCLUDED

#include <ripple/shamap/TreeNodeCache.h>
#include <ripple/shamap/SHAMapTreeNode.h>

namespace ripple {

class SHAMapAbstractNode;

using TreeNodeCache = TaggedCache <uint256, SHAMapAbstractNode>;

} 

#endif








