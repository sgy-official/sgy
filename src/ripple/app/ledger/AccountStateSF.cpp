

#include <ripple/app/ledger/AccountStateSF.h>

namespace ripple {

void
AccountStateSF::gotNode(bool, SHAMapHash const& nodeHash,
    std::uint32_t ledgerSeq, Blob&& nodeData,
    SHAMapTreeNode::TNType) const
{
    db_.store(hotACCOUNT_NODE, std::move(nodeData),
        nodeHash.as_uint256(), ledgerSeq);
}

boost::optional<Blob>
AccountStateSF::getNode(SHAMapHash const& nodeHash) const
{
    return fp_.getFetchPack(nodeHash.as_uint256());
}

} 
























