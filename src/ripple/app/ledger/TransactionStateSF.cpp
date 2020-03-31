

#include <ripple/app/ledger/TransactionStateSF.h>

namespace ripple {

void
TransactionStateSF::gotNode(bool, SHAMapHash const& nodeHash,
    std::uint32_t ledgerSeq, Blob&& nodeData, SHAMapTreeNode::TNType type) const

{
    assert(type != SHAMapTreeNode::tnTRANSACTION_NM);
    db_.store(hotTRANSACTION_NODE, std::move(nodeData),
        nodeHash.as_uint256(), ledgerSeq);
}

boost::optional<Blob>
TransactionStateSF::getNode(SHAMapHash const& nodeHash) const
{
    return fp_.getFetchPack(nodeHash.as_uint256());
}

} 
























