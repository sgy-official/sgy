

#ifndef RIPPLE_APP_MISC_SHAMAPSTORE_H_INCLUDED
#define RIPPLE_APP_MISC_SHAMAPSTORE_H_INCLUDED

#include <ripple/app/ledger/Ledger.h>
#include <ripple/nodestore/Manager.h>
#include <ripple/protocol/ErrorCodes.h>
#include <ripple/core/Stoppable.h>

namespace ripple {

class TransactionMaster;


class SHAMapStore
    : public Stoppable
{
public:
    struct Setup
    {
        explicit Setup() = default;

        bool standalone = false;
        std::uint32_t deleteInterval = 0;
        bool advisoryDelete = false;
        std::uint32_t ledgerHistory = 0;
        Section nodeDatabase;
        std::string databasePath;
        std::uint32_t deleteBatch = 100;
        std::uint32_t backOff = 100;
        std::int32_t ageThreshold = 60;
        Section shardDatabase;
    };

    SHAMapStore (Stoppable& parent) : Stoppable ("SHAMapStore", parent) {}

    
    virtual void onLedgerClosed(std::shared_ptr<Ledger const> const& ledger) = 0;

    virtual void rendezvous() const = 0;

    virtual std::uint32_t clampFetchDepth (std::uint32_t fetch_depth) const = 0;

    virtual std::unique_ptr <NodeStore::Database> makeDatabase (
            std::string const& name,
            std::int32_t readThreads, Stoppable& parent) = 0;

    virtual std::unique_ptr <NodeStore::DatabaseShard> makeDatabaseShard(
        std::string const& name, std::int32_t readThreads,
            Stoppable& parent) = 0;

    
    virtual LedgerIndex setCanDelete (LedgerIndex canDelete) = 0;

    
    virtual bool advisoryDelete() const = 0;

    
    virtual LedgerIndex getLastRotated() = 0;

    
    virtual LedgerIndex getCanDelete() = 0;

    
    virtual int fdlimit() const = 0;
};


SHAMapStore::Setup
setup_SHAMapStore(Config const& c);

std::unique_ptr<SHAMapStore>
make_SHAMapStore(
    Application& app,
    SHAMapStore::Setup const& s,
    Stoppable& parent,
    NodeStore::Scheduler& scheduler,
    beast::Journal journal,
    beast::Journal nodeStoreJournal,
    TransactionMaster& transactionMaster,
    BasicConfig const& conf);
}

#endif








