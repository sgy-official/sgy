

#include <ripple/app/ledger/InboundLedgers.h>
#include <ripple/app/ledger/LedgerMaster.h>
#include <ripple/app/main/Application.h>
#include <ripple/app/misc/NetworkOPs.h>
#include <ripple/basics/DecayingSample.h>
#include <ripple/basics/Log.h>
#include <ripple/core/JobQueue.h>
#include <ripple/nodestore/DatabaseShard.h>
#include <ripple/protocol/jss.h>
#include <ripple/beast/core/LexicalCast.h>
#include <ripple/beast/container/aged_map.h>
#include <memory>
#include <mutex>

namespace ripple {

class InboundLedgersImp
    : public InboundLedgers
    , public Stoppable
{
private:
    Application& app_;
    std::mutex fetchRateMutex_;
    DecayWindow<30, clock_type> fetchRate_;
    beast::Journal j_;

public:
    using u256_acq_pair = std::pair<
        uint256,
        std::shared_ptr <InboundLedger>>;

    static const std::chrono::minutes kReacquireInterval;

    InboundLedgersImp (Application& app, clock_type& clock, Stoppable& parent,
                       beast::insight::Collector::ptr const& collector)
        : Stoppable ("InboundLedgers", parent)
        , app_ (app)
        , fetchRate_(clock.now())
        , j_ (app.journal ("InboundLedger"))
        , m_clock (clock)
        , mRecentFailures (clock)
        , mCounter(collector->make_counter("ledger_fetches"))
    {
    }

    std::shared_ptr<Ledger const>
    acquire(uint256 const& hash, std::uint32_t seq,
        InboundLedger::Reason reason) override
    {
        assert(hash.isNonZero());
        assert(reason != InboundLedger::Reason::SHARD ||
            (seq != 0 && app_.getShardStore()));
        if (isStopping())
            return {};

        bool isNew = true;
        std::shared_ptr<InboundLedger> inbound;
        {
            ScopedLockType sl(mLock);
            auto it = mLedgers.find(hash);
            if (it != mLedgers.end())
            {
                isNew = false;
                inbound = it->second;
            }
            else
            {
                inbound = std::make_shared <InboundLedger>(
                    app_, hash, seq, reason, std::ref(m_clock));
                mLedgers.emplace(hash, inbound);
                inbound->init(sl);
                ++mCounter;
            }
        }

        if (inbound->isFailed())
            return {};

        if (! isNew)
            inbound->update(seq);

        if (! inbound->isComplete())
            return {};

        if (reason == InboundLedger::Reason::HISTORY)
        {
            if (inbound->getLedger()->stateMap().family().isShardBacked())
                app_.getNodeStore().copyLedger(inbound->getLedger());
        }
        else if (reason == InboundLedger::Reason::SHARD)
        {
            auto shardStore = app_.getShardStore();
            if (!shardStore)
            {
                JLOG(j_.error()) <<
                    "Acquiring shard with no shard store available";
                return {};
            }
            if (inbound->getLedger()->stateMap().family().isShardBacked())
                shardStore->setStored(inbound->getLedger());
            else
                shardStore->copyLedger(inbound->getLedger());
        }
        return inbound->getLedger();
    }

    std::shared_ptr<InboundLedger> find (uint256 const& hash) override
    {
        assert (hash.isNonZero ());

        std::shared_ptr<InboundLedger> ret;

        {
            ScopedLockType sl (mLock);

            auto it = mLedgers.find (hash);
            if (it != mLedgers.end ())
            {
                ret = it->second;
            }
        }

        return ret;
    }

    

    
    bool gotLedgerData (LedgerHash const& hash,
            std::shared_ptr<Peer> peer,
            std::shared_ptr<protocol::TMLedgerData> packet_ptr) override
    {
        protocol::TMLedgerData& packet = *packet_ptr;

        JLOG (j_.trace())
            << "Got data (" << packet.nodes ().size ()
            << ") for acquiring ledger: " << hash;

        auto ledger = find (hash);

        if (!ledger)
        {
            JLOG (j_.trace())
                << "Got data for ledger we're no longer acquiring";

            if (packet.type () == protocol::liAS_NODE)
            {
                app_.getJobQueue().addJob(
                    jtLEDGER_DATA, "gotStaleData",
                    [this, packet_ptr] (Job&) { gotStaleData(packet_ptr); });
            }

            return false;
        }

        if (ledger->gotData(std::weak_ptr<Peer>(peer), packet_ptr))
            app_.getJobQueue().addJob (
                jtLEDGER_DATA, "processLedgerData",
                [this, hash] (Job&) { doLedgerData(hash); });

        return true;
    }

    int getFetchCount (int& timeoutCount) override
    {
        timeoutCount = 0;
        int ret = 0;

        std::vector<u256_acq_pair> inboundLedgers;

        {
            ScopedLockType sl (mLock);

            inboundLedgers.reserve(mLedgers.size());
            for (auto const& it : mLedgers)
            {
                inboundLedgers.push_back(it);
            }
        }

        for (auto const& it : inboundLedgers)
        {
            if (it.second->isActive ())
            {
                ++ret;
                timeoutCount += it.second->getTimeouts ();
            }
        }
        return ret;
    }

    void logFailure (uint256 const& h, std::uint32_t seq) override
    {
        ScopedLockType sl (mLock);

        mRecentFailures.emplace(h, seq);
    }

    bool isFailure (uint256 const& h) override
    {
        ScopedLockType sl (mLock);

        beast::expire (mRecentFailures, kReacquireInterval);
        return mRecentFailures.find (h) != mRecentFailures.end();
    }

    void doLedgerData (LedgerHash hash) override
    {
        if (auto ledger = find (hash))
            ledger->runData ();
    }

    
    void gotStaleData (std::shared_ptr<protocol::TMLedgerData> packet_ptr) override
    {
        const uint256 uZero;
        Serializer s;
        try
        {
            for (int i = 0; i < packet_ptr->nodes ().size (); ++i)
            {
                auto const& node = packet_ptr->nodes (i);

                if (!node.has_nodeid () || !node.has_nodedata ())
                    return;

                auto id_string = node.nodeid();
                auto newNode = SHAMapAbstractNode::make(
                    makeSlice(node.nodedata()),
                    0, snfWIRE, SHAMapHash{uZero}, false, app_.journal ("SHAMapNodeID"),
                    SHAMapNodeID(id_string.data(), id_string.size()));

                if (!newNode)
                    return;

                s.erase();
                newNode->addRaw(s, snfPREFIX);

                auto blob = std::make_shared<Blob> (s.begin(), s.end());

                app_.getLedgerMaster().addFetchPack(
                    newNode->getNodeHash().as_uint256(), blob);
            }
        }
        catch (std::exception const&)
        {
        }
    }

    void clearFailures () override
    {
        ScopedLockType sl (mLock);

        mRecentFailures.clear();
        mLedgers.clear();
    }

    std::size_t fetchRate() override
    {
        std::lock_guard<
            std::mutex> lock(fetchRateMutex_);
        return 60 * fetchRate_.value(
            m_clock.now());
    }

    void onLedgerFetched() override
    {
        std::lock_guard<std::mutex> lock(fetchRateMutex_);
        fetchRate_.add(1, m_clock.now());
    }

    Json::Value getInfo() override
    {
        Json::Value ret(Json::objectValue);

        std::vector<u256_acq_pair> acquires;
        {
            ScopedLockType sl (mLock);

            acquires.reserve (mLedgers.size ());
            for (auto const& it : mLedgers)
            {
                assert (it.second);
                acquires.push_back (it);
            }
            for (auto const& it : mRecentFailures)
            {
                if (it.second > 1)
                    ret[beast::lexicalCastThrow <std::string>(
                        it.second)][jss::failed] = true;
                else
                    ret[to_string (it.first)][jss::failed] = true;
            }
        }

        for (auto const& it : acquires)
        {
            std::uint32_t seq = it.second->getSeq();
            if (seq > 1)
                ret[std::to_string(seq)] = it.second->getJson(0);
            else
                ret[to_string (it.first)] = it.second->getJson(0);
        }

        return ret;
    }

    void gotFetchPack () override
    {
        std::vector<std::shared_ptr<InboundLedger>> acquires;
        {
            ScopedLockType sl (mLock);

            acquires.reserve (mLedgers.size ());
            for (auto const& it : mLedgers)
            {
                assert (it.second);
                acquires.push_back (it.second);
            }
        }

        for (auto const& acquire : acquires)
        {
            acquire->checkLocal ();
        }
    }

    void sweep () override
    {
        clock_type::time_point const now (m_clock.now());

        std::vector <MapType::mapped_type> stuffToSweep;
        std::size_t total;
        {
            ScopedLockType sl (mLock);
            MapType::iterator it (mLedgers.begin ());
            total = mLedgers.size ();
            stuffToSweep.reserve (total);

            while (it != mLedgers.end ())
            {
                if (it->second->getLastAction () > now)
                {
                    it->second->touch ();
                    ++it;
                }
                else if ((it->second->getLastAction () +
                          std::chrono::minutes (1)) < now)
                {
                    stuffToSweep.push_back (it->second);
                    it = mLedgers.erase (it);
                }
                else
                {
                    ++it;
                }
            }

            beast::expire (mRecentFailures, kReacquireInterval);

        }

        JLOG (j_.debug()) <<
            "Swept " << stuffToSweep.size () <<
            " out of " << total << " inbound ledgers.";
    }

    void onStop () override
    {
        ScopedLockType lock (mLock);

        mLedgers.clear();
        mRecentFailures.clear();

        stopped();
    }

private:
    clock_type& m_clock;

    using ScopedLockType = std::unique_lock <std::recursive_mutex>;
    std::recursive_mutex mLock;

    using MapType = hash_map <uint256, std::shared_ptr<InboundLedger>>;
    MapType mLedgers;

    beast::aged_map <uint256, std::uint32_t> mRecentFailures;

    beast::insight::Counter mCounter;
};


decltype(InboundLedgersImp::kReacquireInterval)
InboundLedgersImp::kReacquireInterval{5};

InboundLedgers::~InboundLedgers() = default;

std::unique_ptr<InboundLedgers>
make_InboundLedgers (Application& app,
    InboundLedgers::clock_type& clock, Stoppable& parent,
    beast::insight::Collector::ptr const& collector)
{
    return std::make_unique<InboundLedgersImp> (app, clock, parent, collector);
}

} 
























