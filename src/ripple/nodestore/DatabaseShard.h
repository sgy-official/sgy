

#ifndef RIPPLE_NODESTORE_DATABASESHARD_H_INCLUDED
#define RIPPLE_NODESTORE_DATABASESHARD_H_INCLUDED

#include <ripple/nodestore/Database.h>
#include <ripple/app/ledger/Ledger.h>
#include <ripple/basics/RangeSet.h>
#include <ripple/nodestore/Types.h>

#include <boost/optional.hpp>

#include <memory>

namespace ripple {
namespace NodeStore {


class DatabaseShard : public Database
{
public:
    
    DatabaseShard(
        std::string const& name,
        Stoppable& parent,
        Scheduler& scheduler,
        int readThreads,
        Section const& config,
        beast::Journal journal)
        : Database(name, parent, scheduler, readThreads, config, journal)
    {
    }

    
    virtual
    bool
    init() = 0;

    
    virtual
    boost::optional<std::uint32_t>
    prepareLedger(std::uint32_t validLedgerSeq) = 0;

    
    virtual
    bool
    prepareShard(std::uint32_t shardIndex) = 0;

    
    virtual
    void
    removePreShard(std::uint32_t shardIndex) = 0;

    
    virtual
    std::string
    getPreShards() = 0;

    
    virtual
    bool
    importShard(std::uint32_t shardIndex,
        boost::filesystem::path const& srcDir, bool validate) = 0;

    
    virtual
    std::shared_ptr<Ledger>
    fetchLedger(uint256 const& hash, std::uint32_t seq) = 0;

    
    virtual
    void
    setStored(std::shared_ptr<Ledger const> const& ledger) = 0;

    
    virtual
    bool
    contains(std::uint32_t seq) = 0;

    
    virtual
    std::string
    getCompleteShards() = 0;

    
    virtual
    void
    validate() = 0;

    
    virtual
    std::uint32_t
    ledgersPerShard() const = 0;

    
    virtual
    std::uint32_t
    earliestShardIndex() const = 0;

    
    virtual
    std::uint32_t
    seqToShardIndex(std::uint32_t seq) const = 0;

    
    virtual
    std::uint32_t
    firstLedgerSeq(std::uint32_t shardIndex) const = 0;

    
    virtual
    std::uint32_t
    lastLedgerSeq(std::uint32_t shardIndex) const = 0;

    
    virtual
    boost::filesystem::path const&
    getRootDir() const = 0;

    
    static constexpr std::uint32_t ledgersPerShardDefault {16384u};
};

constexpr
std::uint32_t
seqToShardIndex(std::uint32_t seq,
    std::uint32_t ledgersPerShard = DatabaseShard::ledgersPerShardDefault)
{
    return (seq - 1) / ledgersPerShard;
}

}
}

#endif








