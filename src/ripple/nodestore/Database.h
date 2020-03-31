

#ifndef RIPPLE_NODESTORE_DATABASE_H_INCLUDED
#define RIPPLE_NODESTORE_DATABASE_H_INCLUDED

#include <ripple/basics/TaggedCache.h>
#include <ripple/basics/KeyCache.h>
#include <ripple/core/Stoppable.h>
#include <ripple/nodestore/Backend.h>
#include <ripple/nodestore/impl/Tuning.h>
#include <ripple/nodestore/Scheduler.h>
#include <ripple/nodestore/NodeObject.h>
#include <ripple/protocol/SystemParameters.h>

#include <thread>

namespace ripple {

class Ledger;

namespace NodeStore {


class Database : public Stoppable
{
public:
    Database() = delete;

    
    Database(std::string name, Stoppable& parent, Scheduler& scheduler,
        int readThreads, Section const& config, beast::Journal j);

    
    virtual
    ~Database();

    
    virtual
    std::string
    getName() const = 0;

    
    virtual
    void
    import(Database& source) = 0;

    
    virtual
    std::int32_t
    getWriteLoad() const = 0;

    
    virtual
    void
    store(NodeObjectType type, Blob&& data,
        uint256 const& hash, std::uint32_t seq) = 0;

    
    virtual
    std::shared_ptr<NodeObject>
    fetch(uint256 const& hash, std::uint32_t seq) = 0;

    
    virtual
    bool
    asyncFetch(uint256 const& hash, std::uint32_t seq,
        std::shared_ptr<NodeObject>& object) = 0;

    
    virtual
    bool
    copyLedger(std::shared_ptr<Ledger const> const& ledger) = 0;

    
    void
    waitReads();

    
    virtual
    int
    getDesiredAsyncReadCount(std::uint32_t seq) = 0;

    
    virtual
    float
    getCacheHitRate() = 0;

    
    virtual
    void
    tune(int size, std::chrono::seconds age) = 0;

    
    virtual
    void
    sweep() = 0;

    
    std::uint32_t
    getStoreCount() const { return storeCount_; }

    std::uint32_t
    getFetchTotalCount() const { return fetchTotalCount_; }

    std::uint32_t
    getFetchHitCount() const { return fetchHitCount_; }

    std::uint32_t
    getStoreSize() const { return storeSz_; }

    std::uint32_t
    getFetchSize() const { return fetchSz_; }

    
    int
    fdlimit() const { return fdLimit_; }

    void
    onStop() override;

    
    std::uint32_t
    earliestSeq() const
    {
        return earliestSeq_;
    }

protected:
    beast::Journal j_;
    Scheduler& scheduler_;
    int fdLimit_ {0};

    void
    stopThreads();

    void
    storeStats(size_t sz)
    {
        ++storeCount_;
        storeSz_ += sz;
    }

    void
    asyncFetch(uint256 const& hash, std::uint32_t seq,
        std::shared_ptr<TaggedCache<uint256, NodeObject>> const& pCache,
            std::shared_ptr<KeyCache<uint256>> const& nCache);

    std::shared_ptr<NodeObject>
    fetchInternal(uint256 const& hash, Backend& srcBackend);

    void
    importInternal(Backend& dstBackend, Database& srcDB);

    std::shared_ptr<NodeObject>
    doFetch(uint256 const& hash, std::uint32_t seq,
        TaggedCache<uint256, NodeObject>& pCache,
            KeyCache<uint256>& nCache, bool isAsync);

    bool
    copyLedger(Backend& dstBackend, Ledger const& srcLedger,
        std::shared_ptr<TaggedCache<uint256, NodeObject>> const& pCache,
            std::shared_ptr<KeyCache<uint256>> const& nCache,
                std::shared_ptr<Ledger const> const& srcNext);

private:
    std::atomic<std::uint32_t> storeCount_ {0};
    std::atomic<std::uint32_t> fetchTotalCount_ {0};
    std::atomic<std::uint32_t> fetchHitCount_ {0};
    std::atomic<std::uint32_t> storeSz_ {0};
    std::atomic<std::uint32_t> fetchSz_ {0};

    std::mutex readLock_;
    std::condition_variable readCondVar_;
    std::condition_variable readGenCondVar_;

    std::map<uint256, std::tuple<std::uint32_t,
        std::weak_ptr<TaggedCache<uint256, NodeObject>>,
            std::weak_ptr<KeyCache<uint256>>>> read_;

    uint256 readLastHash_;

    std::vector<std::thread> readThreads_;
    bool readShut_ {false};

    uint64_t readGen_ {0};

    std::uint32_t earliestSeq_ {XRP_LEDGER_EARLIEST_SEQ};

    virtual
    std::shared_ptr<NodeObject>
    fetchFrom(uint256 const& hash, std::uint32_t seq) = 0;

    
    virtual
    void
    for_each(std::function <void(std::shared_ptr<NodeObject>)> f) = 0;

    void
    threadEntry();
};

}
}

#endif








