

#ifndef RIPPLE_APP_LEDGER_TRANSACTIONMASTER_H_INCLUDED
#define RIPPLE_APP_LEDGER_TRANSACTIONMASTER_H_INCLUDED

#include <ripple/shamap/SHAMapItem.h>
#include <ripple/shamap/SHAMapTreeNode.h>

namespace ripple {

class Application;
class Transaction;
class STTx;


class TransactionMaster
{
public:
    TransactionMaster (Application& app);
    TransactionMaster (TransactionMaster const&) = delete;
    TransactionMaster& operator= (TransactionMaster const&) = delete;

    std::shared_ptr<Transaction>
    fetch (uint256 const& , bool checkDisk);

    std::shared_ptr<STTx const>
    fetch (std::shared_ptr<SHAMapItem> const& item,
        SHAMapTreeNode::TNType type, bool checkDisk,
            std::uint32_t uCommitLedger);

    bool inLedger (uint256 const& hash, std::uint32_t ledger);

    void canonicalize (std::shared_ptr<Transaction>* pTransaction);

    void sweep (void);

    TaggedCache <uint256, Transaction>&
    getCache();

private:
    Application& mApp;
    TaggedCache <uint256, Transaction> mCache;
};

} 

#endif








