

#include <ripple/nodestore/Database.h>
#include <ripple/app/ledger/Ledger.h>
#include <ripple/basics/chrono.h>
#include <ripple/beast/core/CurrentThreadName.h>
#include <ripple/protocol/HashPrefix.h>

namespace ripple {
namespace NodeStore {

Database::Database(
    std::string name,
    Stoppable& parent,
    Scheduler& scheduler,
    int readThreads,
    Section const& config,
    beast::Journal journal)
    : Stoppable(name, parent)
    , j_(journal)
    , scheduler_(scheduler)
{
    std::uint32_t seq;
    if (get_if_exists<std::uint32_t>(config, "earliest_seq", seq))
    {
        if (seq < 1)
            Throw<std::runtime_error>("Invalid earliest_seq");
        earliestSeq_ = seq;
    }

    while (readThreads-- > 0)
        readThreads_.emplace_back(&Database::threadEntry, this);
}

Database::~Database()
{
    stopThreads();
}

void
Database::waitReads()
{
    std::unique_lock<std::mutex> lock(readLock_);
    std::uint64_t const wakeGen = readGen_ + 2;
    while (! readShut_ && ! read_.empty() && (readGen_ < wakeGen))
        readGenCondVar_.wait(lock);
}

void
Database::onStop()
{
    stopThreads();
    stopped();
}

void
Database::stopThreads()
{
    {
        std::lock_guard <std::mutex> lock(readLock_);
        if (readShut_) 
            return;

        readShut_ = true;
        readCondVar_.notify_all();
        readGenCondVar_.notify_all();
    }

    for (auto& e : readThreads_)
        e.join();
}

void
Database::asyncFetch(uint256 const& hash, std::uint32_t seq,
    std::shared_ptr<TaggedCache<uint256, NodeObject>> const& pCache,
        std::shared_ptr<KeyCache<uint256>> const& nCache)
{
    std::lock_guard <std::mutex> lock(readLock_);
    if (read_.emplace(hash, std::make_tuple(seq, pCache, nCache)).second)
        readCondVar_.notify_one();
}

std::shared_ptr<NodeObject>
Database::fetchInternal(uint256 const& hash, Backend& srcBackend)
{
    std::shared_ptr<NodeObject> nObj;
    Status status;
    try
    {
        status = srcBackend.fetch(hash.begin(), &nObj);
    }
    catch (std::exception const& e)
    {
        JLOG(j_.fatal()) <<
            "Exception, " << e.what();
        Rethrow();
    }

    switch(status)
    {
    case ok:
        ++fetchHitCount_;
        if (nObj)
            fetchSz_ += nObj->getData().size();
        break;
    case notFound:
        break;
    case dataCorrupt:
        JLOG(j_.fatal()) <<
            "Corrupt NodeObject #" << hash;
        break;
    default:
        JLOG(j_.warn()) <<
            "Unknown status=" << status;
        break;
    }
    return nObj;
}

void
Database::importInternal(Backend& dstBackend, Database& srcDB)
{
    Batch b;
    b.reserve(batchWritePreallocationSize);
    srcDB.for_each(
        [&](std::shared_ptr<NodeObject> nObj)
        {
            assert(nObj);
            if (! nObj) 
                return;

            ++storeCount_;
            storeSz_ += nObj->getData().size();

            b.push_back(nObj);
            if (b.size() >= batchWritePreallocationSize)
            {
                dstBackend.storeBatch(b);
                b.clear();
                b.reserve(batchWritePreallocationSize);
            }
        });
    if (! b.empty())
        dstBackend.storeBatch(b);
}

std::shared_ptr<NodeObject>
Database::doFetch(uint256 const& hash, std::uint32_t seq,
    TaggedCache<uint256, NodeObject>& pCache,
        KeyCache<uint256>& nCache, bool isAsync)
{
    FetchReport report;
    report.isAsync = isAsync;
    report.wentToDisk = false;

    using namespace std::chrono;
    auto const before = steady_clock::now();

    auto nObj = pCache.fetch(hash);
    if (! nObj && ! nCache.touch_if_exists(hash))
    {
        report.wentToDisk = true;
        nObj = fetchFrom(hash, seq);
        ++fetchTotalCount_;
        if (! nObj)
        {
            nObj = pCache.fetch(hash);
            if (! nObj)
                nCache.insert(hash);
        }
        else
        {
            pCache.canonicalize(hash, nObj);

            JLOG(j_.trace()) <<
                "HOS: " << hash << " fetch: in db";
        }
    }
    report.wasFound = static_cast<bool>(nObj);
    report.elapsed = duration_cast<milliseconds>(
        steady_clock::now() - before);
    scheduler_.onFetch(report);
    return nObj;
}

bool
Database::copyLedger(Backend& dstBackend, Ledger const& srcLedger,
    std::shared_ptr<TaggedCache<uint256, NodeObject>> const& pCache,
        std::shared_ptr<KeyCache<uint256>> const& nCache,
            std::shared_ptr<Ledger const> const& srcNext)
{
    assert(static_cast<bool>(pCache) == static_cast<bool>(nCache));
    if (srcLedger.info().hash.isZero() ||
        srcLedger.info().accountHash.isZero())
    {
        assert(false);
        JLOG(j_.error()) <<
            "source ledger seq " << srcLedger.info().seq <<
            " is invalid";
        return false;
    }
    auto& srcDB = const_cast<Database&>(
        srcLedger.stateMap().family().db());
    if (&srcDB == this)
    {
        assert(false);
        JLOG(j_.error()) <<
            "source and destination databases are the same";
        return false;
    }

    Batch batch;
    batch.reserve(batchWritePreallocationSize);
    auto storeBatch = [&]() {
#if RIPPLE_VERIFY_NODEOBJECT_KEYS
        for (auto& nObj : batch)
        {
            assert(nObj->getHash() ==
                sha512Hash(makeSlice(nObj->getData())));
            if (pCache && nCache)
            {
                pCache->canonicalize(nObj->getHash(), nObj, true);
                nCache->erase(nObj->getHash());
                storeStats(nObj->getData().size());
            }
        }
#else
        if (pCache && nCache)
            for (auto& nObj : batch)
            {
                pCache->canonicalize(nObj->getHash(), nObj, true);
                nCache->erase(nObj->getHash());
                storeStats(nObj->getData().size());
            }
#endif
        dstBackend.storeBatch(batch);
        batch.clear();
        batch.reserve(batchWritePreallocationSize);
    };
    bool error = false;
    auto f = [&](SHAMapAbstractNode& node) {
        if (auto nObj = srcDB.fetch(
            node.getNodeHash().as_uint256(), srcLedger.info().seq))
        {
            batch.emplace_back(std::move(nObj));
            if (batch.size() >= batchWritePreallocationSize)
                storeBatch();
        }
        else
            error = true;
        return !error;
    };

    {
        Serializer s(1024);
        s.add32(HashPrefix::ledgerMaster);
        addRaw(srcLedger.info(), s);
        auto nObj = NodeObject::createObject(hotLEDGER,
            std::move(s.modData()), srcLedger.info().hash);
        batch.emplace_back(std::move(nObj));
    }

    if (srcLedger.stateMap().getHash().isNonZero())
    {
        if (!srcLedger.stateMap().isValid())
        {
            JLOG(j_.error()) <<
                "source ledger seq " << srcLedger.info().seq <<
                " state map invalid";
            return false;
        }
        if (srcNext && srcNext->info().parentHash == srcLedger.info().hash)
        {
            auto have = srcNext->stateMap().snapShot(false);
            srcLedger.stateMap().snapShot(
                false)->visitDifferences(&(*have), f);
        }
        else
            srcLedger.stateMap().snapShot(false)->visitNodes(f);
        if (error)
            return false;
    }

    if (srcLedger.info().txHash.isNonZero())
    {
        if (!srcLedger.txMap().isValid())
        {
            JLOG(j_.error()) <<
                "source ledger seq " << srcLedger.info().seq <<
                " transaction map invalid";
            return false;
        }
        srcLedger.txMap().snapShot(false)->visitNodes(f);
        if (error)
            return false;
    }

    if (!batch.empty())
        storeBatch();
    return true;
}

void
Database::threadEntry()
{
    beast::setCurrentThreadName("prefetch");
    while (true)
    {
        uint256 lastHash;
        std::uint32_t lastSeq;
        std::shared_ptr<TaggedCache<uint256, NodeObject>> lastPcache;
        std::shared_ptr<KeyCache<uint256>> lastNcache;
        {
            std::unique_lock<std::mutex> lock(readLock_);
            while (! readShut_ && read_.empty())
            {
                readGenCondVar_.notify_all();
                readCondVar_.wait(lock);
            }
            if (readShut_)
                break;

            auto it = read_.lower_bound(readLastHash_);
            if (it == read_.end())
            {
                it = read_.begin();
                ++readGen_;
                readGenCondVar_.notify_all();
            }
            lastHash = it->first;
            lastSeq = std::get<0>(it->second);
            lastPcache = std::get<1>(it->second).lock();
            lastNcache = std::get<2>(it->second).lock();
            read_.erase(it);
            readLastHash_ = lastHash;
        }

        if (lastPcache && lastPcache)
            doFetch(lastHash, lastSeq, *lastPcache, *lastNcache, true);
    }
}

} 
} 
























