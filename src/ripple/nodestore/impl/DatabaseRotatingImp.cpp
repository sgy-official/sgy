

#include <ripple/nodestore/impl/DatabaseRotatingImp.h>
#include <ripple/app/ledger/Ledger.h>
#include <ripple/protocol/HashPrefix.h>

namespace ripple {
namespace NodeStore {

DatabaseRotatingImp::DatabaseRotatingImp(
    std::string const& name,
    Scheduler& scheduler,
    int readThreads,
    Stoppable& parent,
    std::unique_ptr<Backend> writableBackend,
    std::unique_ptr<Backend> archiveBackend,
    Section const& config,
    beast::Journal j)
    : DatabaseRotating(name, parent, scheduler, readThreads, config, j)
    , pCache_(std::make_shared<TaggedCache<uint256, NodeObject>>(
        name, cacheTargetSize, cacheTargetAge, stopwatch(), j))
    , nCache_(std::make_shared<KeyCache<uint256>>(
        name, stopwatch(), cacheTargetSize, cacheTargetAge))
    , writableBackend_(std::move(writableBackend))
    , archiveBackend_(std::move(archiveBackend))
{
    if (writableBackend_)
        fdLimit_ += writableBackend_->fdlimit();
    if (archiveBackend_)
        fdLimit_ += archiveBackend_->fdlimit();
}

std::unique_ptr<Backend>
DatabaseRotatingImp::rotateBackends(
    std::unique_ptr<Backend> newBackend)
{
    auto oldBackend {std::move(archiveBackend_)};
    archiveBackend_ = std::move(writableBackend_);
    writableBackend_ = std::move(newBackend);
    return oldBackend;
}

void
DatabaseRotatingImp::store(NodeObjectType type, Blob&& data,
    uint256 const& hash, std::uint32_t seq)
{
#if RIPPLE_VERIFY_NODEOBJECT_KEYS
    assert(hash == sha512Hash(makeSlice(data)));
#endif
    auto nObj = NodeObject::createObject(type, std::move(data), hash);
    pCache_->canonicalize(hash, nObj, true);
    getWritableBackend()->store(nObj);
    nCache_->erase(hash);
    storeStats(nObj->getData().size());
}

bool
DatabaseRotatingImp::asyncFetch(uint256 const& hash,
    std::uint32_t seq, std::shared_ptr<NodeObject>& object)
{
    object = pCache_->fetch(hash);
    if (object || nCache_->touch_if_exists(hash))
        return true;
    Database::asyncFetch(hash, seq, pCache_, nCache_);
    return false;
}

void
DatabaseRotatingImp::tune(int size, std::chrono::seconds age)
{
    pCache_->setTargetSize(size);
    pCache_->setTargetAge(age);
    nCache_->setTargetSize(size);
    nCache_->setTargetAge(age);
}

void
DatabaseRotatingImp::sweep()
{
    pCache_->sweep();
    nCache_->sweep();
}

std::shared_ptr<NodeObject>
DatabaseRotatingImp::fetchFrom(uint256 const& hash, std::uint32_t seq)
{
    Backends b = getBackends();
    auto nObj = fetchInternal(hash, *b.writableBackend);
    if (! nObj)
    {
        nObj = fetchInternal(hash, *b.archiveBackend);
        if (nObj)
        {
            getWritableBackend()->store(nObj);
            nCache_->erase(hash);
        }
    }
    return nObj;
}

} 
} 
























