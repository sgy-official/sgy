

#ifndef RIPPLE_SHAMAP_FAMILY_H_INCLUDED
#define RIPPLE_SHAMAP_FAMILY_H_INCLUDED

#include <ripple/basics/Log.h>
#include <ripple/shamap/FullBelowCache.h>
#include <ripple/shamap/TreeNodeCache.h>
#include <ripple/nodestore/Database.h>
#include <ripple/beast/utility/Journal.h>
#include <cstdint>

namespace ripple {

class Family
{
public:
    virtual ~Family() = default;

    virtual
    beast::Journal const&
    journal() = 0;

    virtual
    FullBelowCache&
    fullbelow() = 0;

    virtual
    FullBelowCache const&
    fullbelow() const = 0;

    virtual
    TreeNodeCache&
    treecache() = 0;

    virtual
    TreeNodeCache const&
    treecache() const = 0;

    virtual
    NodeStore::Database&
    db() = 0;

    virtual
    NodeStore::Database const&
    db() const = 0;

    virtual
    bool
    isShardBacked() const = 0;

    virtual
    void
    missing_node (std::uint32_t refNum) = 0;

    virtual
    void
    missing_node (uint256 const& refHash, std::uint32_t refNum) = 0;

    virtual
    void
    reset () = 0;
};

} 

#endif








