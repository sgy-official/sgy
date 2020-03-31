

#ifndef RIPPLE_APP_LEDGER_CONSENSUSTRANSSETSF_H_INCLUDED
#define RIPPLE_APP_LEDGER_CONSENSUSTRANSSETSF_H_INCLUDED

#include <ripple/app/main/Application.h>
#include <ripple/shamap/SHAMapSyncFilter.h>
#include <ripple/basics/TaggedCache.h>

namespace ripple {


class ConsensusTransSetSF : public SHAMapSyncFilter
{
public:
    using NodeCache = TaggedCache <SHAMapHash, Blob>;

    ConsensusTransSetSF (Application& app, NodeCache& nodeCache);

    void
    gotNode(bool fromFilter, SHAMapHash const& nodeHash,
        std::uint32_t ledgerSeq, Blob&& nodeData,
            SHAMapTreeNode::TNType type) const override;

    boost::optional<Blob>
    getNode (SHAMapHash const& nodeHash) const override;

private:
    Application& app_;
    NodeCache& m_nodeCache;
    beast::Journal j_;
};

} 

#endif








