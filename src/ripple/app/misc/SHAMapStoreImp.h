

#ifndef RIPPLE_APP_MISC_SHAMAPSTOREIMP_H_INCLUDED
#define RIPPLE_APP_MISC_SHAMAPSTOREIMP_H_INCLUDED

#include <ripple/app/misc/SHAMapStore.h>
#include <ripple/app/ledger/LedgerMaster.h>
#include <ripple/core/DatabaseCon.h>
#include <ripple/nodestore/DatabaseRotating.h>
#include <condition_variable>
#include <thread>

namespace ripple {

class NetworkOPs;

class SHAMapStoreImp : public SHAMapStore
{
private:
    struct SavedState
    {
        std::string writableDb;
        std::string archiveDb;
        LedgerIndex lastRotated;
    };

    enum Health : std::uint8_t
    {
        ok = 0,
        stopping,
        unhealthy
    };

    class SavedStateDB
    {
    public:
        soci::session session_;
        std::mutex mutex_;
        beast::Journal journal_;

        explicit SavedStateDB()
        : journal_ {beast::Journal::getNullSink()}
        { }

        void init (BasicConfig const& config, std::string const& dbName);
        LedgerIndex getCanDelete();
        LedgerIndex setCanDelete (LedgerIndex canDelete);
        SavedState getState();
        void setState (SavedState const& state);
        void setLastRotated (LedgerIndex seq);
    };

    Application& app_;

    std::string const dbName_ = "state";
    std::string const dbPrefix_ = "rippledb";
    std::uint64_t const checkHealthInterval_ = 1000;
    static std::uint32_t const minimumDeletionInterval_ = 256;
    static std::uint32_t const minimumDeletionIntervalSA_ = 8;

    Setup setup_;
    NodeStore::Scheduler& scheduler_;
    beast::Journal journal_;
    beast::Journal nodeStoreJournal_;
    NodeStore::DatabaseRotating* dbRotating_ = nullptr;
    SavedStateDB state_db_;
    std::thread thread_;
    bool stop_ = false;
    bool healthy_ = true;
    mutable std::condition_variable cond_;
    mutable std::condition_variable rendezvous_;
    mutable std::mutex mutex_;
    std::shared_ptr<Ledger const> newLedger_;
    std::atomic<bool> working_;
    TransactionMaster& transactionMaster_;
    std::atomic <LedgerIndex> canDelete_;
    NetworkOPs* netOPs_ = nullptr;
    LedgerMaster* ledgerMaster_ = nullptr;
    FullBelowCache* fullBelowCache_ = nullptr;
    TreeNodeCache* treeNodeCache_ = nullptr;
    DatabaseCon* transactionDb_ = nullptr;
    DatabaseCon* ledgerDb_ = nullptr;
    int fdlimit_ = 0;

public:
    SHAMapStoreImp (Application& app,
            Setup const& setup,
            Stoppable& parent,
            NodeStore::Scheduler& scheduler,
            beast::Journal journal,
            beast::Journal nodeStoreJournal,
            TransactionMaster& transactionMaster,
            BasicConfig const& config);

    ~SHAMapStoreImp()
    {
        if (thread_.joinable())
            thread_.join();
    }

    std::uint32_t
    clampFetchDepth (std::uint32_t fetch_depth) const override
    {
        return setup_.deleteInterval ? std::min (fetch_depth,
                setup_.deleteInterval) : fetch_depth;
    }

    std::unique_ptr <NodeStore::Database> makeDatabase (
            std::string const&name,
            std::int32_t readThreads, Stoppable& parent) override;

    std::unique_ptr <NodeStore::DatabaseShard>
    makeDatabaseShard(std::string const& name,
        std::int32_t readThreads, Stoppable& parent) override;

    LedgerIndex
    setCanDelete (LedgerIndex seq) override
    {
        if (setup_.advisoryDelete)
            canDelete_ = seq;
        return state_db_.setCanDelete (seq);
    }

    bool
    advisoryDelete() const override
    {
        return setup_.advisoryDelete;
    }

    LedgerIndex
    getLastRotated() override
    {
        return state_db_.getState().lastRotated;
    }

    LedgerIndex
    getCanDelete() override
    {
        return canDelete_;
    }

    void onLedgerClosed (std::shared_ptr<Ledger const> const& ledger) override;

    void rendezvous() const override;
    int fdlimit() const override;

private:
    bool copyNode (std::uint64_t& nodeCount, SHAMapAbstractNode const &node);
    void run();
    void dbPaths();

    std::unique_ptr<NodeStore::Backend>
    makeBackendRotating (std::string path = std::string());

    template <class CacheInstance>
    bool
    freshenCache (CacheInstance& cache)
    {
        std::uint64_t check = 0;

        for (auto const& key: cache.getKeys())
        {
            dbRotating_->fetch(key, 0);
            if (! (++check % checkHealthInterval_) && health())
                return true;
        }

        return false;
    }

    
    bool clearSql (DatabaseCon& database, LedgerIndex lastRotated,
                   std::string const& minQuery, std::string const& deleteQuery);
    void clearCaches (LedgerIndex validatedSeq);
    void freshenCaches();
    void clearPrior (LedgerIndex lastRotated);

    Health health();
    void
    onPrepare() override
    {
    }

    void
    onStart() override
    {
        if (setup_.deleteInterval)
            thread_ = std::thread (&SHAMapStoreImp::run, this);
    }

    void onStop() override;
    void onChildrenStopped() override;

};

}

#endif








