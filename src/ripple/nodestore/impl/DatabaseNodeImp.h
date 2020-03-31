

#ifndef RIPPLE_NODESTORE_DATABASENODEIMP_H_INCLUDED
#define RIPPLE_NODESTORE_DATABASENODEIMP_H_INCLUDED

#include <ripple/nodestore/Database.h>
#include <ripple/basics/chrono.h>

namespace ripple {
namespace NodeStore {

class DatabaseNodeImp : public Database
{
public:
    DatabaseNodeImp() = delete;
    DatabaseNodeImp(DatabaseNodeImp const&) = delete;
    DatabaseNodeImp& operator=(DatabaseNodeImp const&) = delete;

    DatabaseNodeImp(
        std::string const& name,
        Scheduler& scheduler,
        int readThreads,
        Stoppable& parent,
        std::unique_ptr<Backend> backend,
        Section const& config,
        beast::Journal j)
        : Database(name, parent, scheduler, readThreads, config, j)
        , pCache_(std::make_shared<TaggedCache<uint256, NodeObject>>(
            name, cacheTargetSize, cacheTargetAge, stopwatch(), j))
        , nCache_(std::make_shared<KeyCache<uint256>>(
            name, stopwatch(), cacheTargetSize, cacheTargetAge))
        , backend_(std::move(backend))
    {
        assert(backend_);
    }

    ~DatabaseNodeImp() override
    {
        stopThreads();
    }

    std::string
    getName() const override
    {
        return backend_->getName();
    }

    std::int32_t
    getWriteLoad() const override
    {
        return backend_->getWriteLoad();
    }

    void
    import(Database& source) override
    {
        importInternal(*backend_.get(), source);
    }

    void
    store(NodeObjectType type, Blob&& data,
        uint256 const& hash, std::uint32_t seq) override;

    std::shared_ptr<NodeObject>
    fetch(uint256 const& hash, std::uint32_t seq) override
    {
        return doFetch(hash, seq, *pCache_, *nCache_, false);
    }

    bool
    asyncFetch(uint256 const& hash, std::uint32_t seq,
        std::shared_ptr<NodeObject>& object) override;

    bool
    copyLedger(std::shared_ptr<Ledger const> const& ledger) override
    {
        return Database::copyLedger(
            *backend_, *ledger, pCache_, nCache_, nullptr);
    }

    int
    getDesiredAsyncReadCount(std::uint32_t seq) override
    {
        return pCache_->getTargetSize() / asyncDivider;
    }

    float
    getCacheHitRate() override {return pCache_->getHitRate();}

    void
    tune(int size, std::chrono::seconds age) override;

    void
    sweep() override;

private:
    std::shared_ptr<TaggedCache<uint256, NodeObject>> pCache_;

    std::shared_ptr<KeyCache<uint256>> nCache_;

    std::unique_ptr<Backend> backend_;

    std::shared_ptr<NodeObject>
    fetchFrom(uint256 const& hash, std::uint32_t seq) override
    {
        return fetchInternal(hash, *backend_);
    }

    void
    for_each(std::function<void(std::shared_ptr<NodeObject>)> f) override
    {
        backend_->for_each(f);
    }
};

}
}

#endif








