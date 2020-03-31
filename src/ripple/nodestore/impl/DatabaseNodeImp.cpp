

#include <ripple/nodestore/impl/DatabaseNodeImp.h>
#include <ripple/app/ledger/Ledger.h>
#include <ripple/protocol/HashPrefix.h>

namespace ripple {
namespace NodeStore {

void
DatabaseNodeImp::store(NodeObjectType type, Blob&& data,
    uint256 const& hash, std::uint32_t seq)
{
#if RIPPLE_VERIFY_NODEOBJECT_KEYS
    assert(hash == sha512Hash(makeSlice(data)));
#endif
    auto nObj = NodeObject::createObject(type, std::move(data), hash);
    pCache_->canonicalize(hash, nObj, true);
    backend_->store(nObj);
    nCache_->erase(hash);
    storeStats(nObj->getData().size());
}

bool
DatabaseNodeImp::asyncFetch(uint256 const& hash,
    std::uint32_t seq, std::shared_ptr<NodeObject>& object)
{
    object = pCache_->fetch(hash);
    if (object || nCache_->touch_if_exists(hash))
        return true;
    Database::asyncFetch(hash, seq, pCache_, nCache_);
    return false;
}

void
DatabaseNodeImp::tune(int size, std::chrono::seconds age)
{
    pCache_->setTargetSize(size);
    pCache_->setTargetAge(age);
    nCache_->setTargetSize(size);
    nCache_->setTargetAge(age);
}

void
DatabaseNodeImp::sweep()
{
    pCache_->sweep();
    nCache_->sweep();
}

} 
} 
























