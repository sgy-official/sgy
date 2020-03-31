

#ifndef RIPPLE_SHAMAP_SHAMAPSYNCFILTER_H_INCLUDED
#define RIPPLE_SHAMAP_SHAMAPSYNCFILTER_H_INCLUDED

#include <ripple/shamap/SHAMapNodeID.h>
#include <ripple/shamap/SHAMapTreeNode.h>
#include <boost/optional.hpp>


namespace ripple {

class SHAMapSyncFilter
{
public:
    virtual ~SHAMapSyncFilter () = default;
    SHAMapSyncFilter() = default;
    SHAMapSyncFilter(SHAMapSyncFilter const&) = delete;
    SHAMapSyncFilter& operator=(SHAMapSyncFilter const&) = delete;

    virtual
    void
    gotNode(bool fromFilter, SHAMapHash const& nodeHash,
        std::uint32_t ledgerSeq, Blob&& nodeData,
            SHAMapTreeNode::TNType type) const = 0;

    virtual
    boost::optional<Blob>
    getNode(SHAMapHash const& nodeHash) const = 0;
};

}

#endif








