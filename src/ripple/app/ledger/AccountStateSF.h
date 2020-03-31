

#ifndef RIPPLE_APP_LEDGER_ACCOUNTSTATESF_H_INCLUDED
#define RIPPLE_APP_LEDGER_ACCOUNTSTATESF_H_INCLUDED

#include <ripple/app/ledger/AbstractFetchPackContainer.h>
#include <ripple/nodestore/Database.h>
#include <ripple/shamap/SHAMapSyncFilter.h>

namespace ripple {

class AccountStateSF : public SHAMapSyncFilter
{
public:
    AccountStateSF(NodeStore::Database& db, AbstractFetchPackContainer& fp)
        : db_(db)
        , fp_(fp)
    {}

    void
    gotNode(bool fromFilter, SHAMapHash const& nodeHash,
        std::uint32_t ledgerSeq, Blob&& nodeData,
            SHAMapTreeNode::TNType type) const override;

    boost::optional<Blob>
    getNode(SHAMapHash const& nodeHash) const override;

private:
    NodeStore::Database& db_;
    AbstractFetchPackContainer& fp_;
};

} 

#endif








