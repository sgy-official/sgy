

#ifndef RIPPLE_NODESTORE_SHARD_H_INCLUDED
#define RIPPLE_NODESTORE_SHARD_H_INCLUDED

#include <ripple/app/ledger/Ledger.h>
#include <ripple/basics/BasicConfig.h>
#include <ripple/basics/RangeSet.h>
#include <ripple/nodestore/NodeObject.h>
#include <ripple/nodestore/Scheduler.h>

#include <nudb/nudb.hpp>
#include <boost/filesystem.hpp>
#include <boost/serialization/map.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

namespace ripple {
namespace NodeStore {

inline static
bool
removeAll(boost::filesystem::path const& path, beast::Journal& j)
{
    try
    {
        boost::filesystem::remove_all(path);
    }
    catch (std::exception const& e)
    {
        JLOG(j.error()) <<
            "exception: " << e.what();
        return false;
    }
    return true;
}

using PCache = TaggedCache<uint256, NodeObject>;
using NCache = KeyCache<uint256>;
class DatabaseShard;


class Shard
{
public:
    Shard(DatabaseShard const& db, std::uint32_t index, int cacheSz,
        std::chrono::seconds cacheAge, beast::Journal& j);

    bool
    open(Section config, Scheduler& scheduler, nudb::context& ctx);

    bool
    setStored(std::shared_ptr<Ledger const> const& l);

    boost::optional<std::uint32_t>
    prepare();

    bool
    contains(std::uint32_t seq) const;

    bool
    validate(Application& app);

    std::uint32_t
    index() const {return index_;}

    bool
    complete() const {return complete_;}

    std::shared_ptr<PCache>&
    pCache() {return pCache_;}

    std::shared_ptr<NCache>&
    nCache() {return nCache_;}

    std::uint64_t
    fileSize() const {return fileSize_;}

    std::shared_ptr<Backend> const&
    getBackend() const
    {
        assert(backend_);
        return backend_;
    }

    std::uint32_t
    fdlimit() const
    {
        assert(backend_);
        return backend_->fdlimit();
    }

    std::shared_ptr<Ledger const>
    lastStored() {return lastStored_;}

private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & storedSeqs_;
    }

    static constexpr auto controlFileName = "control.txt";

    std::uint32_t const index_;

    std::uint32_t const firstSeq_;

    std::uint32_t const lastSeq_;

    std::uint32_t const maxLedgers_;

    std::shared_ptr<PCache> pCache_;

    std::shared_ptr<NCache> nCache_;

    boost::filesystem::path const dir_;

    boost::filesystem::path const control_;

    std::uint64_t fileSize_ {0};
    std::shared_ptr<Backend> backend_;
    beast::Journal j_;

    bool complete_ {false};

    RangeSet<std::uint32_t> storedSeqs_;

    std::shared_ptr<Ledger const> lastStored_;

    bool
    valLedger(std::shared_ptr<Ledger const> const& l,
        std::shared_ptr<Ledger const> const& next);

    std::shared_ptr<NodeObject>
    valFetch(uint256 const& hash);

    void
    updateFileSize();

    bool
    saveControl();
};

} 
} 

#endif








