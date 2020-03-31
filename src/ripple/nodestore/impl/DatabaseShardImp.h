

#ifndef RIPPLE_NODESTORE_DATABASESHARDIMP_H_INCLUDED
#define RIPPLE_NODESTORE_DATABASESHARDIMP_H_INCLUDED

#include <ripple/nodestore/DatabaseShard.h>
#include <ripple/nodestore/impl/Shard.h>

namespace ripple {
namespace NodeStore {

class DatabaseShardImp : public DatabaseShard
{
public:
    DatabaseShardImp() = delete;
    DatabaseShardImp(DatabaseShardImp const&) = delete;
    DatabaseShardImp(DatabaseShardImp&&) = delete;
    DatabaseShardImp& operator=(DatabaseShardImp const&) = delete;
    DatabaseShardImp& operator=(DatabaseShardImp&&) = delete;

    DatabaseShardImp(
        Application& app,
        std::string const& name,
        Stoppable& parent,
        Scheduler& scheduler,
        int readThreads,
        Section const& config,
        beast::Journal j);

    ~DatabaseShardImp() override;

    bool
    init() override;

    boost::optional<std::uint32_t>
    prepareLedger(std::uint32_t validLedgerSeq) override;

    bool
    prepareShard(std::uint32_t shardIndex) override;

    void
    removePreShard(std::uint32_t shardIndex) override;

    std::string
    getPreShards() override;

    bool
    importShard(std::uint32_t shardIndex,
        boost::filesystem::path const& srcDir, bool validate) override;

    std::shared_ptr<Ledger>
    fetchLedger(uint256 const& hash, std::uint32_t seq) override;

    void
    setStored(std::shared_ptr<Ledger const> const& ledger) override;

    bool
    contains(std::uint32_t seq) override;

    std::string
    getCompleteShards() override;

    void
    validate() override;

    std::uint32_t
    ledgersPerShard() const override
    {
        return ledgersPerShard_;
    }

    std::uint32_t
    earliestShardIndex() const override
    {
        return earliestShardIndex_;
    }

    std::uint32_t
    seqToShardIndex(std::uint32_t seq) const override
    {
        assert(seq >= earliestSeq());
        return NodeStore::seqToShardIndex(seq, ledgersPerShard_);
    }

    std::uint32_t
    firstLedgerSeq(std::uint32_t shardIndex) const override
    {
        assert(shardIndex >= earliestShardIndex_);
        if (shardIndex <= earliestShardIndex_)
            return earliestSeq();
        return 1 + (shardIndex * ledgersPerShard_);
    }

    std::uint32_t
    lastLedgerSeq(std::uint32_t shardIndex) const override
    {
        assert(shardIndex >= earliestShardIndex_);
        return (shardIndex + 1) * ledgersPerShard_;
    }

    boost::filesystem::path const&
    getRootDir() const override
    {
        return dir_;
    }

    std::string
    getName() const override
    {
        return backendName_;
    }

    
    void
    import(Database& source) override;

    std::int32_t
    getWriteLoad() const override;

    void
    store(NodeObjectType type, Blob&& data,
        uint256 const& hash, std::uint32_t seq) override;

    std::shared_ptr<NodeObject>
    fetch(uint256 const& hash, std::uint32_t seq) override;

    bool
    asyncFetch(uint256 const& hash, std::uint32_t seq,
        std::shared_ptr<NodeObject>& object) override;

    bool
    copyLedger(std::shared_ptr<Ledger const> const& ledger) override;

    int
    getDesiredAsyncReadCount(std::uint32_t seq) override;

    float
    getCacheHitRate() override;

    void
    tune(int size, std::chrono::seconds age) override;

    void
    sweep() override;

private:
    Application& app_;
    mutable std::mutex m_;
    bool init_ {false};

    std::unique_ptr<nudb::context> ctx_;

    std::map<std::uint32_t, std::unique_ptr<Shard>> complete_;

    std::unique_ptr<Shard> incomplete_;

    std::map<std::uint32_t, Shard*> preShards_;

    Section const config_;
    boost::filesystem::path const dir_;

    bool canAdd_ {true};

    std::string status_;

    bool backed_;

    std::string const backendName_;

    std::uint64_t const maxDiskSpace_;

    std::uint64_t usedDiskSpace_ {0};

    std::uint32_t const ledgersPerShard_;

    std::uint32_t const earliestShardIndex_;

    std::uint64_t avgShardSz_;

    int cacheSz_ {shardCacheSz};
    std::chrono::seconds cacheAge_ {shardCacheAge};

    static constexpr auto importMarker_ = "import";

    std::shared_ptr<NodeObject>
    fetchFrom(uint256 const& hash, std::uint32_t seq) override;

    void
    for_each(std::function <void(std::shared_ptr<NodeObject>)> f) override
    {
        Throw<std::runtime_error>("Shard store import not supported");
    }

    boost::optional<std::uint32_t>
    findShardIndexToAdd(std::uint32_t validLedgerSeq,
        std::lock_guard<std::mutex>&);

    void
    updateStats(std::lock_guard<std::mutex>&);

    std::pair<std::shared_ptr<PCache>, std::shared_ptr<NCache>>
    selectCache(std::uint32_t seq);

    int
    calcTargetCacheSz(std::lock_guard<std::mutex>&) const
    {
        return std::max(shardCacheSz, cacheSz_ / std::max(
            1, static_cast<int>(complete_.size() + (incomplete_ ? 1 : 0))));
    }

    std::uint64_t
    available() const;
};

} 
} 

#endif








