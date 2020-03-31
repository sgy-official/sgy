

#ifndef RIPPLE_APP_LEDGER_LEDGERMASTER_H_INCLUDED
#define RIPPLE_APP_LEDGER_LEDGERMASTER_H_INCLUDED

#include <ripple/app/main/Application.h>
#include <ripple/app/ledger/AbstractFetchPackContainer.h>
#include <ripple/app/ledger/InboundLedgers.h>
#include <ripple/app/ledger/Ledger.h>
#include <ripple/app/ledger/LedgerCleaner.h>
#include <ripple/app/ledger/LedgerHistory.h>
#include <ripple/app/ledger/LedgerHolder.h>
#include <ripple/app/ledger/LedgerReplay.h>
#include <ripple/app/misc/CanonicalTXSet.h>
#include <ripple/basics/chrono.h>
#include <ripple/basics/RangeSet.h>
#include <ripple/basics/ScopedLock.h>
#include <ripple/basics/StringUtilities.h>
#include <ripple/protocol/RippleLedgerHash.h>
#include <ripple/protocol/STValidation.h>
#include <ripple/beast/insight/Collector.h>
#include <ripple/core/Stoppable.h>
#include <ripple/protocol/Protocol.h>
#include <ripple/beast/utility/PropertyStream.h>
#include <mutex>

#include <ripple/protocol/messages.h>

namespace ripple {

class Peer;
class Transaction;




class LedgerMaster
    : public Stoppable
    , public AbstractFetchPackContainer
{
public:
    explicit
    LedgerMaster(Application& app, Stopwatch& stopwatch,
        Stoppable& parent,
            beast::insight::Collector::ptr const& collector,
                beast::Journal journal);

    virtual ~LedgerMaster () = default;

    LedgerIndex getCurrentLedgerIndex ();
    LedgerIndex getValidLedgerIndex ();

    bool isCompatible (
        ReadView const&,
        beast::Journal::Stream,
        char const* reason);

    std::recursive_mutex& peekMutex ();

    std::shared_ptr<ReadView const>
    getCurrentLedger();

    std::shared_ptr<Ledger const>
    getClosedLedger()
    {
        return mClosedLedger.get();
    }

    std::shared_ptr<Ledger const>
    getValidatedLedger ()
    {
        return mValidLedger.get();
    }

    Rules getValidatedRules();

    std::shared_ptr<ReadView const>
    getPublishedLedger();

    std::chrono::seconds getPublishedLedgerAge ();
    std::chrono::seconds getValidatedLedgerAge ();
    bool isCaughtUp(std::string& reason);

    std::uint32_t getEarliestFetch ();

    bool storeLedger (std::shared_ptr<Ledger const> ledger);

    void setFullLedger (
        std::shared_ptr<Ledger const> const& ledger,
            bool isSynchronous, bool isCurrent);

    
    bool canBeCurrent (std::shared_ptr<Ledger const> const& ledger);

    void switchLCL (std::shared_ptr<Ledger const> const& lastClosed);

    void failedSave(std::uint32_t seq, uint256 const& hash);

    std::string getCompleteLedgers ();

    
    void applyHeldTransactions ();

    
    std::vector<std::shared_ptr<STTx const>>
    pruneHeldTransactions(AccountID const& account,
        std::uint32_t const seq);

    
    uint256 getHashBySeq (std::uint32_t index);

    
    boost::optional<LedgerHash> walkHashBySeq (std::uint32_t index);

    
    boost::optional<LedgerHash> walkHashBySeq (
        std::uint32_t index,
        std::shared_ptr<ReadView const> const& referenceLedger);

    std::shared_ptr<Ledger const>
    getLedgerBySeq (std::uint32_t index);

    std::shared_ptr<Ledger const>
    getLedgerByHash (uint256 const& hash);

    void setLedgerRangePresent (
        std::uint32_t minV, std::uint32_t maxV);

    boost::optional<LedgerHash> getLedgerHash(
        std::uint32_t desiredSeq,
        std::shared_ptr<ReadView const> const& knownGoodLedger);

    boost::optional <NetClock::time_point> getCloseTimeBySeq (
        LedgerIndex ledgerIndex);

    boost::optional <NetClock::time_point> getCloseTimeByHash (
        LedgerHash const& ledgerHash, LedgerIndex ledgerIndex);

    void addHeldTransaction (std::shared_ptr<Transaction> const& trans);
    void fixMismatch (ReadView const& ledger);

    bool haveLedger (std::uint32_t seq);
    void clearLedger (std::uint32_t seq);
    bool getValidatedRange (
        std::uint32_t& minVal, std::uint32_t& maxVal);
    bool getFullValidatedRange (
        std::uint32_t& minVal, std::uint32_t& maxVal);

    void tune (int size, std::chrono::seconds age);
    void sweep ();
    float getCacheHitRate ();

    void checkAccept (std::shared_ptr<Ledger const> const& ledger);
    void checkAccept (uint256 const& hash, std::uint32_t seq);
    void
    consensusBuilt(
        std::shared_ptr<Ledger const> const& ledger,
        uint256 const& consensusHash,
        Json::Value consensus);

    LedgerIndex getBuildingLedger ();
    void setBuildingLedger (LedgerIndex index);

    void tryAdvance ();
    bool newPathRequest (); 
    bool isNewPathRequest ();
    bool newOrderBookDB (); 

    bool fixIndex (
        LedgerIndex ledgerIndex, LedgerHash const& ledgerHash);
    void doLedgerCleaner(Json::Value const& parameters);

    beast::PropertyStream::Source& getPropertySource ();

    void clearPriorLedgers (LedgerIndex seq);

    void clearLedgerCachePrior (LedgerIndex seq);

    void takeReplay (std::unique_ptr<LedgerReplay> replay);
    std::unique_ptr<LedgerReplay> releaseReplay ();

    void gotFetchPack (
        bool progress,
        std::uint32_t seq);

    void addFetchPack (
        uint256 const& hash,
        std::shared_ptr<Blob>& data);

    boost::optional<Blob>
    getFetchPack (uint256 const& hash) override;

    void makeFetchPack (
        std::weak_ptr<Peer> const& wPeer,
        std::shared_ptr<protocol::TMGetObjectByHash> const& request,
        uint256 haveLedgerHash,
        UptimeClock::time_point uptime);

    std::size_t getFetchPackCacheSize () const;

    bool
    haveValidated()
    {
        return !mValidLedger.empty();
    }

private:
    using ScopedLockType = std::lock_guard <std::recursive_mutex>;
    using ScopedUnlockType = GenericScopedUnlock <std::recursive_mutex>;

    void setValidLedger(
        std::shared_ptr<Ledger const> const& l);
    void setPubLedger(
        std::shared_ptr<Ledger const> const& l);

    void tryFill(
        Job& job,
        std::shared_ptr<Ledger const> ledger);

    void getFetchPack(
        LedgerIndex missing, InboundLedger::Reason reason);

    boost::optional<LedgerHash> getLedgerHashForHistory(
        LedgerIndex index, InboundLedger::Reason reason);

    std::size_t getNeededValidations();
    void advanceThread();
    void fetchForHistory(
        std::uint32_t missing,
        bool& progress,
        InboundLedger::Reason reason);
    void doAdvance(ScopedLockType&);
    bool shouldAcquire(
        std::uint32_t const currentLedger,
        std::uint32_t const ledgerHistory,
        std::uint32_t const ledgerHistoryIndex,
        std::uint32_t const candidateLedger) const;

    std::vector<std::shared_ptr<Ledger const>>
    findNewLedgersToPublish();

    void updatePaths(Job& job);

    bool newPFWork(const char *name, ScopedLockType&);

private:
    Application& app_;
    beast::Journal m_journal;

    std::recursive_mutex mutable m_mutex;

    LedgerHolder mClosedLedger;

    LedgerHolder mValidLedger;

    std::shared_ptr<Ledger const> mPubLedger;

    std::shared_ptr<Ledger const> mPathLedger;

    std::shared_ptr<Ledger const> mHistLedger;

    std::shared_ptr<Ledger const> mShardLedger;

    std::pair <uint256, LedgerIndex> mLastValidLedger {uint256(), 0};

    LedgerHistory mLedgerHistory;

    CanonicalTXSet mHeldTransactions {uint256()};

    std::unique_ptr<LedgerReplay> replayData;

    std::recursive_mutex mCompleteLock;
    RangeSet<std::uint32_t> mCompleteLedgers;

    std::unique_ptr <detail::LedgerCleaner> mLedgerCleaner;

    bool                        mAdvanceThread {false};

    bool                        mAdvanceWork {false};
    int                         mFillInProgress {0};

    int     mPathFindThread {0};    
    bool    mPathFindNewRequest {false};

    std::atomic_flag mGotFetchPackThread = ATOMIC_FLAG_INIT; 

    std::atomic <std::uint32_t> mPubLedgerClose {0};
    std::atomic <LedgerIndex> mPubLedgerSeq {0};
    std::atomic <std::uint32_t> mValidLedgerSign {0};
    std::atomic <LedgerIndex> mValidLedgerSeq {0};
    std::atomic <LedgerIndex> mBuildingLedgerSeq {0};

    bool const standalone_;

    std::uint32_t const fetch_depth_;

    std::uint32_t const ledger_history_;

    std::uint32_t const ledger_fetch_size_;

    TaggedCache<uint256, Blob> fetch_packs_;

    std::uint32_t fetch_seq_ {0};

    LedgerIndex const max_ledger_difference_ {1000000};

};

} 

#endif








