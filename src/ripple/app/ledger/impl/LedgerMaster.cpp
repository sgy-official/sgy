

#include <ripple/app/ledger/LedgerMaster.h>
#include <ripple/app/ledger/OpenLedger.h>
#include <ripple/app/ledger/OrderBookDB.h>
#include <ripple/app/ledger/PendingSaves.h>
#include <ripple/app/tx/apply.h>
#include <ripple/app/main/Application.h>
#include <ripple/app/misc/AmendmentTable.h>
#include <ripple/app/misc/HashRouter.h>
#include <ripple/app/misc/LoadFeeTrack.h>
#include <ripple/app/misc/NetworkOPs.h>
#include <ripple/app/misc/SHAMapStore.h>
#include <ripple/app/misc/Transaction.h>
#include <ripple/app/misc/TxQ.h>
#include <ripple/app/consensus/RCLValidations.h>
#include <ripple/app/misc/ValidatorList.h>
#include <ripple/app/paths/PathRequests.h>
#include <ripple/basics/contract.h>
#include <ripple/basics/Log.h>
#include <ripple/basics/TaggedCache.h>
#include <ripple/basics/UptimeClock.h>
#include <ripple/core/TimeKeeper.h>
#include <ripple/nodestore/DatabaseShard.h>
#include <ripple/overlay/Overlay.h>
#include <ripple/overlay/Peer.h>
#include <ripple/protocol/digest.h>
#include <ripple/protocol/HashPrefix.h>
#include <ripple/resource/Fees.h>
#include <algorithm>
#include <cassert>
#include <memory>
#include <vector>

namespace ripple {

using namespace std::chrono_literals;

#define MAX_LEDGER_GAP          100

auto constexpr MAX_LEDGER_AGE_ACQUIRE = 1min;

auto constexpr MAX_WRITE_LOAD_ACQUIRE = 8192;

LedgerMaster::LedgerMaster (Application& app, Stopwatch& stopwatch,
    Stoppable& parent,
    beast::insight::Collector::ptr const& collector, beast::Journal journal)
    : Stoppable ("LedgerMaster", parent)
    , app_ (app)
    , m_journal (journal)
    , mLedgerHistory (collector, app)
    , mLedgerCleaner (detail::make_LedgerCleaner (
        app, *this, app_.journal("LedgerCleaner")))
    , standalone_ (app_.config().standalone())
    , fetch_depth_ (app_.getSHAMapStore ().clampFetchDepth (
        app_.config().FETCH_DEPTH))
    , ledger_history_ (app_.config().LEDGER_HISTORY)
    , ledger_fetch_size_ (app_.config().getSize (siLedgerFetch))
    , fetch_packs_ ("FetchPack", 65536, 45s, stopwatch,
        app_.journal("TaggedCache"))
{
}

LedgerIndex
LedgerMaster::getCurrentLedgerIndex ()
{
    return app_.openLedger().current()->info().seq;
}

LedgerIndex
LedgerMaster::getValidLedgerIndex ()
{
    return mValidLedgerSeq;
}

bool
LedgerMaster::isCompatible (
    ReadView const& view,
    beast::Journal::Stream s,
    char const* reason)
{
    auto validLedger = getValidatedLedger();

    if (validLedger &&
        ! areCompatible (*validLedger, view, s, reason))
    {
        return false;
    }

    {
        ScopedLockType sl (m_mutex);

        if ((mLastValidLedger.second != 0) &&
            ! areCompatible (mLastValidLedger.first,
                mLastValidLedger.second, view, s, reason))
        {
            return false;
        }
    }

    return true;
}

std::chrono::seconds
LedgerMaster::getPublishedLedgerAge()
{
    std::chrono::seconds pubClose{mPubLedgerClose.load()};
    if (pubClose == 0s)
    {
        JLOG (m_journal.debug()) << "No published ledger";
        return weeks{2};
    }

    std::chrono::seconds ret = app_.timeKeeper().closeTime().time_since_epoch();
    ret -= pubClose;
    ret = (ret > 0s) ? ret : 0s;

    JLOG (m_journal.trace()) << "Published ledger age is " << ret.count();
    return ret;
}

std::chrono::seconds
LedgerMaster::getValidatedLedgerAge()
{
    std::chrono::seconds valClose{mValidLedgerSign.load()};
    if (valClose == 0s)
    {
        JLOG (m_journal.debug()) << "No validated ledger";
        return weeks{2};
    }

    std::chrono::seconds ret = app_.timeKeeper().closeTime().time_since_epoch();
    ret -= valClose;
    ret = (ret > 0s) ? ret : 0s;

    JLOG (m_journal.trace()) << "Validated ledger age is " << ret.count();
    return ret;
}

bool
LedgerMaster::isCaughtUp(std::string& reason)
{
    if (getPublishedLedgerAge() > 3min)
    {
        reason = "No recently-published ledger";
        return false;
    }
    std::uint32_t validClose = mValidLedgerSign.load();
    std::uint32_t pubClose = mPubLedgerClose.load();
    if (!validClose || !pubClose)
    {
        reason = "No published ledger";
        return false;
    }
    if (validClose  > (pubClose + 90))
    {
        reason = "Published ledger lags validated ledger";
        return false;
    }
    return true;
}

void
LedgerMaster::setValidLedger(
    std::shared_ptr<Ledger const> const& l)
{

    std::vector <NetClock::time_point> times;
    boost::optional<uint256> consensusHash;

    if (! standalone_)
    {
        auto const vals = app_.getValidations().getTrustedForLedger(l->info().hash);
        times.reserve(vals.size());
        for(auto const& val: vals)
            times.push_back(val->getSignTime());

        if(!vals.empty())
            consensusHash = vals.front()->getConsensusHash();
    }

    NetClock::time_point signTime;

    if (! times.empty () && times.size() >= app_.validators ().quorum ())
    {
        std::sort (times.begin (), times.end ());
        auto const t0 = times[(times.size() - 1) / 2];
        auto const t1 = times[times.size() / 2];
        signTime = t0 + (t1 - t0)/2;
    }
    else
    {
        signTime = l->info().closeTime;
    }

    mValidLedger.set (l);
    mValidLedgerSign = signTime.time_since_epoch().count();
    assert (mValidLedgerSeq ||
            !app_.getMaxDisallowedLedger() ||
            l->info().seq + max_ledger_difference_ >
                    app_.getMaxDisallowedLedger());
    (void) max_ledger_difference_;
    mValidLedgerSeq = l->info().seq;

    app_.getOPs().updateLocalTx (*l);
    app_.getSHAMapStore().onLedgerClosed (getValidatedLedger());
    mLedgerHistory.validatedLedger (l, consensusHash);
    app_.getAmendmentTable().doValidatedLedger (l);
    if (!app_.getOPs().isAmendmentBlocked() &&
        app_.getAmendmentTable().hasUnsupportedEnabled ())
    {
        JLOG (m_journal.error()) <<
            "One or more unsupported amendments activated: server blocked.";
        app_.getOPs().setAmendmentBlocked();
    }
}

void
LedgerMaster::setPubLedger(
    std::shared_ptr<Ledger const> const& l)
{
    mPubLedger = l;
    mPubLedgerClose = l->info().closeTime.time_since_epoch().count();
    mPubLedgerSeq = l->info().seq;
}

void
LedgerMaster::addHeldTransaction (
    std::shared_ptr<Transaction> const& transaction)
{
    ScopedLockType ml (m_mutex);
    mHeldTransactions.insert (transaction->getSTransaction ());
}

bool
LedgerMaster::canBeCurrent (std::shared_ptr<Ledger const> const& ledger)
{
    assert (ledger);


    auto validLedger = getValidatedLedger();
    if (validLedger &&
        (ledger->info().seq < validLedger->info().seq))
    {
        JLOG (m_journal.trace()) << "Candidate for current ledger has low seq "
            << ledger->info().seq << " < " << validLedger->info().seq;
        return false;
    }


    auto closeTime = app_.timeKeeper().closeTime();
    auto ledgerClose = ledger->info().parentCloseTime;

    if ((validLedger || (ledger->info().seq > 10)) &&
        ((std::max (closeTime, ledgerClose) - std::min (closeTime, ledgerClose))
            > 5min))
    {
        JLOG (m_journal.warn()) << "Candidate for current ledger has close time "
            << to_string(ledgerClose) << " at network time "
            << to_string(closeTime) << " seq " << ledger->info().seq;
        return false;
    }

    if (validLedger)
    {

        LedgerIndex maxSeq = validLedger->info().seq + 10;

        if (closeTime > validLedger->info().parentCloseTime)
            maxSeq += std::chrono::duration_cast<std::chrono::seconds>(
                closeTime - validLedger->info().parentCloseTime).count() / 2;

        if (ledger->info().seq > maxSeq)
        {
            JLOG (m_journal.warn()) << "Candidate for current ledger has high seq "
                << ledger->info().seq << " > " << maxSeq;
            return false;
        }

        JLOG (m_journal.trace()) << "Acceptable seq range: " <<
            validLedger->info().seq << " <= " <<
            ledger->info().seq << " <= " << maxSeq;
    }

    return true;
}

void
LedgerMaster::switchLCL(std::shared_ptr<Ledger const> const& lastClosed)
{
    assert (lastClosed);
    if(! lastClosed->isImmutable())
        LogicError("mutable ledger in switchLCL");

    if (lastClosed->open())
        LogicError ("The new last closed ledger is open!");

    {
        ScopedLockType ml (m_mutex);
        mClosedLedger.set (lastClosed);
    }

    if (standalone_)
    {
        setFullLedger (lastClosed, true, false);
        tryAdvance();
    }
    else
    {
        checkAccept (lastClosed);
    }
}

bool
LedgerMaster::fixIndex (LedgerIndex ledgerIndex, LedgerHash const& ledgerHash)
{
    return mLedgerHistory.fixIndex (ledgerIndex, ledgerHash);
}

bool
LedgerMaster::storeLedger (std::shared_ptr<Ledger const> ledger)
{
    return mLedgerHistory.insert(std::move(ledger), false);
}


void
LedgerMaster::applyHeldTransactions ()
{
    ScopedLockType sl (m_mutex);

    app_.openLedger().modify(
        [&](OpenView& view, beast::Journal j)
        {
            bool any = false;
            for (auto const& it : mHeldTransactions)
            {
                ApplyFlags flags = tapNONE;
                auto const result = app_.getTxQ().apply(
                    app_, view, it.second, flags, j);
                if (result.second)
                    any = true;
            }
            return any;
        });

    mHeldTransactions.reset (
        app_.openLedger().current()->info().parentHash);
}

std::vector<std::shared_ptr<STTx const>>
LedgerMaster::pruneHeldTransactions(AccountID const& account,
    std::uint32_t const seq)
{
    ScopedLockType sl(m_mutex);

    return mHeldTransactions.prune(account, seq);
}

LedgerIndex
LedgerMaster::getBuildingLedger ()
{
    return mBuildingLedgerSeq.load ();
}

void
LedgerMaster::setBuildingLedger (LedgerIndex i)
{
    mBuildingLedgerSeq.store (i);
}

bool
LedgerMaster::haveLedger (std::uint32_t seq)
{
    ScopedLockType sl (mCompleteLock);
    return boost::icl::contains(mCompleteLedgers, seq);
}

void
LedgerMaster::clearLedger (std::uint32_t seq)
{
    ScopedLockType sl (mCompleteLock);
    mCompleteLedgers.erase (seq);
}

bool
LedgerMaster::getFullValidatedRange (std::uint32_t& minVal, std::uint32_t& maxVal)
{
    maxVal = mPubLedgerSeq.load();

    if (!maxVal)
        return false;

    boost::optional<std::uint32_t> maybeMin;
    {
        ScopedLockType sl (mCompleteLock);
        maybeMin = prevMissing(mCompleteLedgers, maxVal);
    }

    if (maybeMin == boost::none)
        minVal = maxVal;
    else
        minVal = 1 + *maybeMin;

    return true;
}

bool
LedgerMaster::getValidatedRange (std::uint32_t& minVal, std::uint32_t& maxVal)
{
    if (!getFullValidatedRange(minVal, maxVal))
        return false;


    auto const pendingSaves =
        app_.pendingSaves().getSnapshot();

    if (!pendingSaves.empty() && ((minVal != 0) || (maxVal != 0)))
    {
        while (pendingSaves.count(maxVal) > 0)
            --maxVal;
        while (pendingSaves.count(minVal) > 0)
            ++minVal;

        for(auto v : pendingSaves)
        {
            if ((v.first >= minVal) && (v.first <= maxVal))
            {
                if (v.first > ((minVal + maxVal) / 2))
                    maxVal = v.first - 1;
                else
                    minVal = v.first + 1;
            }
        }

        if (minVal > maxVal)
            minVal = maxVal = 0;
    }

    return true;
}

std::uint32_t
LedgerMaster::getEarliestFetch()
{
    std::uint32_t e = getClosedLedger()->info().seq;

    if (e > fetch_depth_)
        e -= fetch_depth_;
    else
        e = 0;
    return e;
}

void
LedgerMaster::tryFill (
    Job& job,
    std::shared_ptr<Ledger const> ledger)
{
    std::uint32_t seq = ledger->info().seq;
    uint256 prevHash = ledger->info().parentHash;

    std::map< std::uint32_t, std::pair<uint256, uint256> > ledgerHashes;

    std::uint32_t minHas = seq;
    std::uint32_t maxHas = seq;

    NodeStore::Database& nodeStore {app_.getNodeStore()};
    while (! job.shouldCancel() && seq > 0)
    {
        {
            ScopedLockType ml (m_mutex);
            minHas = seq;
            --seq;

            if (haveLedger (seq))
                break;
        }

        auto it (ledgerHashes.find (seq));

        if (it == ledgerHashes.end ())
        {
            if (app_.isShutdown ())
                return;

            {
                ScopedLockType ml(mCompleteLock);
                mCompleteLedgers.insert(range(minHas, maxHas));
            }
            maxHas = minHas;
            ledgerHashes = getHashesByIndex ((seq < 500)
                ? 0
                : (seq - 499), seq, app_);
            it = ledgerHashes.find (seq);

            if (it == ledgerHashes.end ())
                break;

            if (!nodeStore.fetch(ledgerHashes.begin()->second.first,
                ledgerHashes.begin()->first))
            {
                JLOG(m_journal.warn()) <<
                    "SQL DB ledger sequence " << seq <<
                    " mismatches node store";
                break;
            }
        }

        if (it->second.first != prevHash)
            break;

        prevHash = it->second.second;
    }

    {
        ScopedLockType ml (mCompleteLock);
        mCompleteLedgers.insert(range(minHas, maxHas));
    }
    {
        ScopedLockType ml (m_mutex);
        mFillInProgress = 0;
        tryAdvance();
    }
}


void
LedgerMaster::getFetchPack (LedgerIndex missing,
    InboundLedger::Reason reason)
{
    auto haveHash {getLedgerHashForHistory(missing + 1, reason)};
    if (!haveHash || haveHash->isZero())
    {
        if (reason == InboundLedger::Reason::SHARD)
        {
            auto const shardStore {app_.getShardStore()};
            auto const shardIndex {shardStore->seqToShardIndex(missing)};
            if (missing < shardStore->lastLedgerSeq(shardIndex))
            {
                 JLOG(m_journal.error())
                    << "No hash for fetch pack. "
                    << "Missing ledger sequence " << missing
                    << " while acquiring shard " << shardIndex;
            }
        }
        else
        {
            JLOG(m_journal.error()) <<
                "No hash for fetch pack. Missing Index " << missing;
        }
        return;
    }

    std::shared_ptr<Peer> target;
    {
        int maxScore = 0;
        auto peerList = app_.overlay ().getActivePeers();
        for (auto const& peer : peerList)
        {
            if (peer->hasRange (missing, missing + 1))
            {
                int score = peer->getScore (true);
                if (! target || (score > maxScore))
                {
                    target = peer;
                    maxScore = score;
                }
            }
        }
    }

    if (target)
    {
        protocol::TMGetObjectByHash tmBH;
        tmBH.set_query (true);
        tmBH.set_type (protocol::TMGetObjectByHash::otFETCH_PACK);
        tmBH.set_ledgerhash (haveHash->begin(), 32);
        auto packet = std::make_shared<Message> (
            tmBH, protocol::mtGET_OBJECTS);

        target->send (packet);
        JLOG(m_journal.trace()) << "Requested fetch pack for " << missing;
    }
    else
        JLOG (m_journal.debug()) << "No peer for fetch pack";
}

void
LedgerMaster::fixMismatch (ReadView const& ledger)
{
    int invalidate = 0;
    boost::optional<uint256> hash;

    for (std::uint32_t lSeq = ledger.info().seq - 1; lSeq > 0; --lSeq)
    {
        if (haveLedger (lSeq))
        {
            try
            {
                hash = hashOfSeq(ledger, lSeq, m_journal);
            }
            catch (std::exception const&)
            {
                JLOG (m_journal.warn()) <<
                    "fixMismatch encounters partial ledger";
                clearLedger(lSeq);
                return;
            }

            if (hash)
            {
                auto otherLedger = getLedgerBySeq (lSeq);

                if (otherLedger && (otherLedger->info().hash == *hash))
                {
                    if (invalidate != 0)
                    {
                        JLOG (m_journal.warn())
                            << "Match at " << lSeq
                            << ", " << invalidate
                            << " prior ledgers invalidated";
                    }

                    return;
                }
            }

            clearLedger (lSeq);
            ++invalidate;
        }
    }

    if (invalidate != 0)
    {
        JLOG (m_journal.warn()) <<
            "All " << invalidate << " prior ledgers invalidated";
    }
}

void
LedgerMaster::setFullLedger (
    std::shared_ptr<Ledger const> const& ledger,
        bool isSynchronous, bool isCurrent)
{
    JLOG (m_journal.debug()) <<
        "Ledger " << ledger->info().seq <<
        " accepted :" << ledger->info().hash;
    assert (ledger->stateMap().getHash ().isNonZero ());

    ledger->setValidated();
    ledger->setFull();

    if (isCurrent)
        mLedgerHistory.insert(ledger, true);

    {
        uint256 prevHash = getHashByIndex (ledger->info().seq - 1, app_);
        if (prevHash.isNonZero () && prevHash != ledger->info().parentHash)
            clearLedger (ledger->info().seq - 1);
    }


    pendSaveValidated (app_, ledger, isSynchronous, isCurrent);

    {
        ScopedLockType ml (mCompleteLock);
        mCompleteLedgers.insert (ledger->info().seq);
    }

    {
        ScopedLockType ml (m_mutex);

        if (ledger->info().seq > mValidLedgerSeq)
            setValidLedger(ledger);
        if (!mPubLedger)
        {
            setPubLedger(ledger);
            app_.getOrderBookDB().setup(ledger);
        }

        if (ledger->info().seq != 0 && haveLedger (ledger->info().seq - 1))
        {
            auto prevLedger = getLedgerBySeq (ledger->info().seq - 1);

            if (!prevLedger ||
                (prevLedger->info().hash != ledger->info().parentHash))
            {
                JLOG (m_journal.warn())
                    << "Acquired ledger invalidates previous ledger: "
                    << (prevLedger ? "hashMismatch" : "missingLedger");
                fixMismatch (*ledger);
            }
        }
    }
}

void
LedgerMaster::failedSave(std::uint32_t seq, uint256 const& hash)
{
    clearLedger(seq);
    app_.getInboundLedgers().acquire(
        hash, seq, InboundLedger::Reason::GENERIC);
}

void
LedgerMaster::checkAccept (uint256 const& hash, std::uint32_t seq)
{
    std::size_t valCount = 0;

    if (seq != 0)
    {
        if (seq < mValidLedgerSeq)
            return;

        valCount =
            app_.getValidations().numTrustedForLedger (hash);

        if (valCount >= app_.validators ().quorum ())
        {
            ScopedLockType ml (m_mutex);
            if (seq > mLastValidLedger.second)
                mLastValidLedger = std::make_pair (hash, seq);
        }

        if (seq == mValidLedgerSeq)
            return;

        if (seq == mBuildingLedgerSeq)
            return;
    }

    auto ledger = mLedgerHistory.getLedgerByHash (hash);

    if (!ledger)
    {
        if ((seq != 0) && (getValidLedgerIndex() == 0))
        {
            if (valCount >= app_.validators ().quorum ())
                app_.overlay().checkSanity (seq);
        }

        ledger = app_.getInboundLedgers().acquire(
            hash, seq, InboundLedger::Reason::GENERIC);
    }

    if (ledger)
        checkAccept (ledger);
}


std::size_t
LedgerMaster::getNeededValidations ()
{
    return standalone_ ? 0 : app_.validators().quorum ();
}

void
LedgerMaster::checkAccept (
    std::shared_ptr<Ledger const> const& ledger)
{

    if (! canBeCurrent (ledger))
        return;

    ScopedLockType ml (m_mutex);

    if (ledger->info().seq <= mValidLedgerSeq)
        return;

    auto const minVal = getNeededValidations();
    auto const tvc = app_.getValidations().numTrustedForLedger(ledger->info().hash);
    if (tvc < minVal) 
    {
        JLOG (m_journal.trace()) <<
            "Only " << tvc <<
            " validations for " << ledger->info().hash;
        return;
    }

    JLOG (m_journal.info())
        << "Advancing accepted ledger to " << ledger->info().seq
        << " with >= " << minVal << " validations";

    ledger->setValidated();
    ledger->setFull();
    setValidLedger(ledger);
    if (!mPubLedger)
    {
        pendSaveValidated(app_, ledger, true, true);
        setPubLedger(ledger);
        app_.getOrderBookDB().setup(ledger);
    }

    std::uint32_t const base = app_.getFeeTrack().getLoadBase();
    auto fees = app_.getValidations().fees (ledger->info().hash, base);
    {
        auto fees2 = app_.getValidations().fees (
            ledger->info(). parentHash, base);
        fees.reserve (fees.size() + fees2.size());
        std::copy (fees2.begin(), fees2.end(), std::back_inserter(fees));
    }
    std::uint32_t fee;
    if (! fees.empty())
    {
        std::sort (fees.begin(), fees.end());
        fee = fees[fees.size() / 2]; 
    }
    else
    {
        fee = base;
    }

    app_.getFeeTrack().setRemoteFee(fee);

    tryAdvance ();
}


void
LedgerMaster::consensusBuilt(
    std::shared_ptr<Ledger const> const& ledger,
    uint256 const& consensusHash,
    Json::Value consensus)
{

    setBuildingLedger (0);

    if (standalone_)
        return;

    mLedgerHistory.builtLedger (ledger, consensusHash, std::move (consensus));

    if (ledger->info().seq <= mValidLedgerSeq)
    {
        auto stream = app_.journal ("LedgerConsensus").info();
        JLOG (stream)
            << "Consensus built old ledger: "
            << ledger->info().seq << " <= " << mValidLedgerSeq;
        return;
    }

    checkAccept (ledger);

    if (ledger->info().seq <= mValidLedgerSeq)
    {
        auto stream = app_.journal ("LedgerConsensus").debug();
        JLOG (stream)
            << "Consensus ledger fully validated";
        return;
    }


    auto const val =
        app_.getValidations().currentTrusted();

    class valSeq
    {
        public:

        valSeq () : valCount_ (0), ledgerSeq_ (0) { ; }

        void mergeValidation (LedgerIndex seq)
        {
            valCount_++;

            if (ledgerSeq_ == 0)
                ledgerSeq_ = seq;
        }

        std::size_t valCount_;
        LedgerIndex ledgerSeq_;
    };

    hash_map <uint256, valSeq> count;
    for (auto const& v : val)
    {
        valSeq& vs = count[v->getLedgerHash()];
        vs.mergeValidation (v->getFieldU32 (sfLedgerSequence));
    }

    auto const neededValidations = getNeededValidations ();
    auto maxSeq = mValidLedgerSeq.load();
    auto maxLedger = ledger->info().hash;

    for (auto& v : count)
        if (v.second.valCount_ > neededValidations)
        {
            if (v.second.ledgerSeq_ == 0)
            {
                if (auto ledger = getLedgerByHash (v.first))
                    v.second.ledgerSeq_ = ledger->info().seq;
            }

            if (v.second.ledgerSeq_ > maxSeq)
            {
                maxSeq = v.second.ledgerSeq_;
                maxLedger = v.first;
            }
        }

    if (maxSeq > mValidLedgerSeq)
    {
        auto stream = app_.journal ("LedgerConsensus").debug();
        JLOG (stream)
            << "Consensus triggered check of ledger";
        checkAccept (maxLedger, maxSeq);
    }
}

void
LedgerMaster::advanceThread()
{
    ScopedLockType sl (m_mutex);
    assert (!mValidLedger.empty () && mAdvanceThread);

    JLOG (m_journal.trace()) << "advanceThread<";

    try
    {
        doAdvance(sl);
    }
    catch (std::exception const&)
    {
        JLOG (m_journal.fatal()) << "doAdvance throws an exception";
    }

    mAdvanceThread = false;
    JLOG (m_journal.trace()) << "advanceThread>";
}

boost::optional<LedgerHash>
LedgerMaster::getLedgerHashForHistory(
    LedgerIndex index, InboundLedger::Reason reason)
{
    boost::optional<LedgerHash> ret;
    auto const& l {reason == InboundLedger::Reason::SHARD ?
        mShardLedger : mHistLedger};

    if (l && l->info().seq >= index)
    {
        ret = hashOfSeq(*l, index, m_journal);
        if (! ret)
            ret = walkHashBySeq (index, l);
    }

    if (! ret)
        ret = walkHashBySeq (index);

    return ret;
}

std::vector<std::shared_ptr<Ledger const>>
LedgerMaster::findNewLedgersToPublish ()
{
    std::vector<std::shared_ptr<Ledger const>> ret;

    JLOG (m_journal.trace()) << "findNewLedgersToPublish<";

    if (mValidLedger.empty ())
    {
        JLOG(m_journal.trace()) <<
            "No valid journal, nothing to publish.";
        return {};
    }

    if (! mPubLedger)
    {
        JLOG(m_journal.info()) <<
            "First published ledger will be " << mValidLedgerSeq;
        return { mValidLedger.get () };
    }

    if (mValidLedgerSeq > (mPubLedgerSeq + MAX_LEDGER_GAP))
    {
        JLOG(m_journal.warn()) <<
            "Gap in validated ledger stream " << mPubLedgerSeq <<
            " - " << mValidLedgerSeq - 1;

        auto valLedger = mValidLedger.get ();
        ret.push_back (valLedger);
        setPubLedger (valLedger);
        app_.getOrderBookDB().setup(valLedger);

        return { valLedger };
    }

    if (mValidLedgerSeq <= mPubLedgerSeq)
    {
        JLOG(m_journal.trace()) <<
            "No valid journal, nothing to publish.";
        return {};
    }

    int acqCount = 0;

    auto pubSeq = mPubLedgerSeq + 1; 
    auto valLedger = mValidLedger.get ();
    std::uint32_t valSeq = valLedger->info().seq;

    ScopedUnlockType sul(m_mutex);
    try
    {
        for (std::uint32_t seq = pubSeq; seq <= valSeq; ++seq)
        {
            JLOG(m_journal.trace())
                << "Trying to fetch/publish valid ledger " << seq;

            std::shared_ptr<Ledger const> ledger;
            auto hash = hashOfSeq(*valLedger, seq, m_journal);
            if (! hash)
                hash = beast::zero; 
            if (seq == valSeq)
            {
                ledger = valLedger;
            }
            else if (hash->isZero())
            {
                JLOG (m_journal.fatal())
                    << "Ledger: " << valSeq
                    << " does not have hash for " << seq;
                assert (false);
            }
            else
            {
                ledger = mLedgerHistory.getLedgerByHash (*hash);
            }

            if (! ledger && (++acqCount < ledger_fetch_size_))
                ledger = app_.getInboundLedgers ().acquire(
                    *hash, seq, InboundLedger::Reason::GENERIC);

            if (ledger && (ledger->info().seq == pubSeq))
            {
                ledger->setValidated();
                ret.push_back (ledger);
                ++pubSeq;
            }
        }

        JLOG(m_journal.trace()) <<
            "ready to publish " << ret.size() << " ledgers.";
    }
    catch (std::exception const&)
    {
        JLOG(m_journal.error()) <<
            "Exception while trying to find ledgers to publish.";
    }

    return ret;
}

void
LedgerMaster::tryAdvance()
{
    ScopedLockType ml (m_mutex);

    mAdvanceWork = true;
    if (!mAdvanceThread && !mValidLedger.empty ())
    {
        mAdvanceThread = true;
        app_.getJobQueue ().addJob (
            jtADVANCE, "advanceLedger",
            [this] (Job&) { advanceThread(); });
    }
}

boost::optional<LedgerHash>
LedgerMaster::getLedgerHash(
    std::uint32_t desiredSeq,
    std::shared_ptr<ReadView const> const& knownGoodLedger)
{
    assert(desiredSeq < knownGoodLedger->info().seq);

    auto hash = hashOfSeq(*knownGoodLedger, desiredSeq, m_journal);

    if (! hash)
    {
        std::uint32_t seq = (desiredSeq + 255) % 256;
        assert(seq < desiredSeq);

        hash = hashOfSeq(*knownGoodLedger, seq, m_journal);
        if (hash)
        {
            if (auto l = getLedgerByHash(*hash))
            {
                hash = hashOfSeq(*l, desiredSeq, m_journal);
                assert (hash);
            }
        }
        else
        {
            assert(false);
        }
    }

    return hash;
}

void
LedgerMaster::updatePaths (Job& job)
{
    {
        ScopedLockType ml (m_mutex);
        if (app_.getOPs().isNeedNetworkLedger())
        {
            --mPathFindThread;
            return;
        }
    }


    while (! job.shouldCancel())
    {
        std::shared_ptr<ReadView const> lastLedger;
        {
            ScopedLockType ml (m_mutex);

            if (!mValidLedger.empty() &&
                (!mPathLedger ||
                    (mPathLedger->info().seq != mValidLedgerSeq)))
            { 
                mPathLedger = mValidLedger.get ();
                lastLedger = mPathLedger;
            }
            else if (mPathFindNewRequest)
            { 
                lastLedger = app_.openLedger().current();
            }
            else
            { 
                --mPathFindThread;
                return;
            }
        }

        if (!standalone_)
        { 
            using namespace std::chrono;
            auto age = time_point_cast<seconds>(app_.timeKeeper().closeTime())
                - lastLedger->info().closeTime;
            if (age > 1min)
            {
                JLOG (m_journal.debug())
                    << "Published ledger too old for updating paths";
                ScopedLockType ml (m_mutex);
                --mPathFindThread;
                return;
            }
        }

        try
        {
            app_.getPathRequests().updateAll(
                lastLedger, job.getCancelCallback());
        }
        catch (SHAMapMissingNode&)
        {
            JLOG (m_journal.info())
                << "Missing node detected during pathfinding";
            if (lastLedger->open())
            {
                app_.getInboundLedgers().acquire(
                    lastLedger->info().parentHash,
                    lastLedger->info().seq - 1,
                    InboundLedger::Reason::GENERIC);
            }
            else
            {
                app_.getInboundLedgers().acquire(
                    lastLedger->info().hash,
                    lastLedger->info().seq,
                    InboundLedger::Reason::GENERIC);
            }
        }
    }
}

bool
LedgerMaster::newPathRequest ()
{
    ScopedLockType ml (m_mutex);
    mPathFindNewRequest = newPFWork("pf:newRequest", ml);
    return mPathFindNewRequest;
}

bool
LedgerMaster::isNewPathRequest ()
{
    ScopedLockType ml (m_mutex);
    bool const ret = mPathFindNewRequest;
    mPathFindNewRequest = false;
    return ret;
}

bool
LedgerMaster::newOrderBookDB ()
{
    ScopedLockType ml (m_mutex);
    mPathLedger.reset();

    return newPFWork("pf:newOBDB", ml);
}


bool
LedgerMaster::newPFWork (const char *name, ScopedLockType&)
{
    if (mPathFindThread < 2)
    {
        if (app_.getJobQueue().addJob (
            jtUPDATE_PF, name,
            [this] (Job& j) { updatePaths(j); }))
        {
            ++mPathFindThread;
        }
    }
    return mPathFindThread > 0 && !isStopping();
}

std::recursive_mutex&
LedgerMaster::peekMutex ()
{
    return m_mutex;
}

std::shared_ptr<ReadView const>
LedgerMaster::getCurrentLedger ()
{
    return app_.openLedger().current();
}

Rules
LedgerMaster::getValidatedRules ()
{

    if (auto const ledger = getValidatedLedger())
        return ledger->rules();

    return Rules(app_.config().features);
}

std::shared_ptr<ReadView const>
LedgerMaster::getPublishedLedger ()
{
    ScopedLockType lock(m_mutex);
    return mPubLedger;
}

std::string
LedgerMaster::getCompleteLedgers ()
{
    ScopedLockType sl (mCompleteLock);
    return to_string(mCompleteLedgers);
}

boost::optional <NetClock::time_point>
LedgerMaster::getCloseTimeBySeq (LedgerIndex ledgerIndex)
{
    uint256 hash = getHashBySeq (ledgerIndex);
    return hash.isNonZero() ? getCloseTimeByHash(
        hash, ledgerIndex) : boost::none;
}

boost::optional <NetClock::time_point>
LedgerMaster::getCloseTimeByHash(LedgerHash const& ledgerHash,
    std::uint32_t index)
{
    auto node = app_.getNodeStore().fetch(ledgerHash, index);
    if (node &&
        (node->getData().size() >= 120))
    {
        SerialIter it (node->getData().data(), node->getData().size());
        if (it.get32() == HashPrefix::ledgerMaster)
        {
            it.skip (
                4+8+32+    
                32+32+4);  
            return NetClock::time_point{NetClock::duration{it.get32()}};
        }
    }

    return boost::none;
}

uint256
LedgerMaster::getHashBySeq (std::uint32_t index)
{
    uint256 hash = mLedgerHistory.getLedgerHash (index);

    if (hash.isNonZero ())
        return hash;

    return getHashByIndex (index, app_);
}

boost::optional<LedgerHash>
LedgerMaster::walkHashBySeq (std::uint32_t index)
{
    boost::optional<LedgerHash> ledgerHash;

    if (auto referenceLedger = mValidLedger.get ())
        ledgerHash = walkHashBySeq (index, referenceLedger);

    return ledgerHash;
}

boost::optional<LedgerHash>
LedgerMaster::walkHashBySeq (
    std::uint32_t index,
    std::shared_ptr<ReadView const> const& referenceLedger)
{
    if (!referenceLedger || (referenceLedger->info().seq < index))
    {
        return boost::none;
    }

    auto ledgerHash = hashOfSeq(*referenceLedger, index, m_journal);
    if (ledgerHash)
        return ledgerHash;

    LedgerIndex refIndex = getCandidateLedger(index);
    auto const refHash = hashOfSeq(*referenceLedger, refIndex, m_journal);
    assert(refHash);
    if (refHash)
    {
        auto ledger = mLedgerHistory.getLedgerByHash (*refHash);

        if (ledger)
        {
            try
            {
                ledgerHash = hashOfSeq(*ledger, index, m_journal);
            }
            catch(SHAMapMissingNode&)
            {
                ledger.reset();
            }
        }

        if (!ledger)
        {
            auto const ledger = app_.getInboundLedgers().acquire (
                *refHash, refIndex, InboundLedger::Reason::GENERIC);
            if (ledger)
            {
                ledgerHash = hashOfSeq(*ledger, index, m_journal);
                assert (ledgerHash);
            }
        }
    }
    return ledgerHash;
}

std::shared_ptr<Ledger const>
LedgerMaster::getLedgerBySeq (std::uint32_t index)
{
    if (index <= mValidLedgerSeq)
    {
        if (auto valid = mValidLedger.get ())
        {
            if (valid->info().seq == index)
                return valid;

            try
            {
                auto const hash = hashOfSeq(*valid, index, m_journal);

                if (hash)
                    return mLedgerHistory.getLedgerByHash (*hash);
            }
            catch (std::exception const&)
            {
            }
        }
    }

    if (auto ret = mLedgerHistory.getLedgerBySeq (index))
        return ret;

    auto ret = mClosedLedger.get ();
    if (ret && (ret->info().seq == index))
        return ret;

    clearLedger (index);
    return {};
}

std::shared_ptr<Ledger const>
LedgerMaster::getLedgerByHash (uint256 const& hash)
{
    if (auto ret = mLedgerHistory.getLedgerByHash (hash))
        return ret;

    auto ret = mClosedLedger.get ();
    if (ret && (ret->info().hash == hash))
        return ret;

    return {};
}

void
LedgerMaster::doLedgerCleaner(Json::Value const& parameters)
{
    mLedgerCleaner->doClean (parameters);
}

void
LedgerMaster::setLedgerRangePresent (std::uint32_t minV, std::uint32_t maxV)
{
    ScopedLockType sl (mCompleteLock);
    mCompleteLedgers.insert(range(minV, maxV));
}

void
LedgerMaster::tune (int size, std::chrono::seconds age)
{
    mLedgerHistory.tune (size, age);
}

void
LedgerMaster::sweep ()
{
    mLedgerHistory.sweep ();
    fetch_packs_.sweep ();
}

float
LedgerMaster::getCacheHitRate ()
{
    return mLedgerHistory.getCacheHitRate ();
}

beast::PropertyStream::Source&
LedgerMaster::getPropertySource ()
{
    return *mLedgerCleaner;
}

void
LedgerMaster::clearPriorLedgers (LedgerIndex seq)
{
    ScopedLockType sl(mCompleteLock);
    if (seq > 0)
        mCompleteLedgers.erase(range(0u, seq - 1));
}

void
LedgerMaster::clearLedgerCachePrior (LedgerIndex seq)
{
    mLedgerHistory.clearLedgerCachePrior (seq);
}

void
LedgerMaster::takeReplay (std::unique_ptr<LedgerReplay> replay)
{
    replayData = std::move (replay);
}

std::unique_ptr<LedgerReplay>
LedgerMaster::releaseReplay ()
{
    return std::move (replayData);
}

bool
LedgerMaster::shouldAcquire (
    std::uint32_t const currentLedger,
    std::uint32_t const ledgerHistory,
    std::uint32_t const ledgerHistoryIndex,
    std::uint32_t const candidateLedger) const
{


    bool ret (candidateLedger >= currentLedger ||
        ((ledgerHistoryIndex > 0) &&
            (candidateLedger > ledgerHistoryIndex)) ||
        (currentLedger - candidateLedger) <= ledgerHistory);

    JLOG (m_journal.trace())
        << "Missing ledger "
        << candidateLedger
        << (ret ? " should" : " should NOT")
        << " be acquired";
    return ret;
}

void
LedgerMaster::fetchForHistory(
    std::uint32_t missing,
    bool& progress,
    InboundLedger::Reason reason)
{
    ScopedUnlockType sl(m_mutex);
    if (auto hash = getLedgerHashForHistory(missing, reason))
    {
        assert(hash->isNonZero());
        auto ledger = getLedgerByHash(*hash);
        if (! ledger)
        {
            if (!app_.getInboundLedgers().isFailure(*hash))
            {
                ledger = app_.getInboundLedgers().acquire(
                    *hash, missing, reason);
                if (!ledger &&
                    missing != fetch_seq_ &&
                    missing > app_.getNodeStore().earliestSeq())
                {
                    JLOG(m_journal.trace())
                        << "fetchForHistory want fetch pack " << missing;
                    fetch_seq_ = missing;
                    getFetchPack(missing, reason);
                }
                else
                    JLOG(m_journal.trace())
                        << "fetchForHistory no fetch pack for " << missing;
            }
            else
                JLOG(m_journal.debug())
                    << "fetchForHistory found failed acquire";
        }
        if (ledger)
        {
            auto seq = ledger->info().seq;
            assert(seq == missing);
            JLOG(m_journal.trace()) <<
                "fetchForHistory acquired " << seq;
            if (reason == InboundLedger::Reason::SHARD)
            {
                ledger->setFull();
                {
                    ScopedLockType lock(m_mutex);
                    mShardLedger = ledger;
                }
                if (!ledger->stateMap().family().isShardBacked())
                    app_.getShardStore()->copyLedger(ledger);
            }
            else
            {
                setFullLedger(ledger, false, false);
                int fillInProgress;
                {
                    ScopedLockType lock(m_mutex);
                    mHistLedger = ledger;
                    fillInProgress = mFillInProgress;
                }
                if (fillInProgress == 0 &&
                    getHashByIndex(seq - 1, app_) == ledger->info().parentHash)
                {
                    {
                        ScopedLockType lock(m_mutex);
                        mFillInProgress = seq;
                    }
                    app_.getJobQueue().addJob(jtADVANCE, "tryFill",
                        [this, ledger](Job& j) { tryFill(j, ledger); });
                }
            }
            progress = true;
        }
        else
        {
            std::uint32_t fetchSz;
            if (reason == InboundLedger::Reason::SHARD)
                fetchSz = app_.getShardStore()->firstLedgerSeq(
                    app_.getShardStore()->seqToShardIndex(missing));
            else
                fetchSz = app_.getNodeStore().earliestSeq();
            fetchSz = missing >= fetchSz ?
                std::min(ledger_fetch_size_, (missing - fetchSz) + 1) : 0;
            try
            {
                for (std::uint32_t i = 0; i < fetchSz; ++i)
                {
                    std::uint32_t seq = missing - i;
                    if (auto h = getLedgerHashForHistory(seq, reason))
                    {
                        assert(h->isNonZero());
                        app_.getInboundLedgers().acquire(*h, seq, reason);
                    }
                }
            }
            catch (std::exception const&)
            {
                JLOG(m_journal.warn()) << "Threw while prefetching";
            }
        }
    }
    else
    {
        JLOG(m_journal.fatal()) << "Can't find ledger following prevMissing "
                                << missing;
        JLOG(m_journal.fatal()) << "Pub:" << mPubLedgerSeq
                                << " Val:" << mValidLedgerSeq;
        JLOG(m_journal.fatal()) << "Ledgers: "
                                << app_.getLedgerMaster().getCompleteLedgers();
        JLOG(m_journal.fatal()) << "Acquire reason: "
            << (reason == InboundLedger::Reason::HISTORY ? "HISTORY" : "SHARD");
        clearLedger(missing + 1);
        progress = true;
    }
}

void LedgerMaster::doAdvance (ScopedLockType& sl)
{
    do
    {
        mAdvanceWork = false; 
        bool progress = false;

        auto const pubLedgers = findNewLedgersToPublish ();
        if (pubLedgers.empty())
        {
            if (!standalone_ && !app_.getFeeTrack().isLoadedLocal() &&
                (app_.getJobQueue().getJobCount(jtPUBOLDLEDGER) < 10) &&
                (mValidLedgerSeq == mPubLedgerSeq) &&
                (getValidatedLedgerAge() < MAX_LEDGER_AGE_ACQUIRE) &&
                (app_.getNodeStore().getWriteLoad() < MAX_WRITE_LOAD_ACQUIRE))
            {
                InboundLedger::Reason reason = InboundLedger::Reason::HISTORY;
                boost::optional<std::uint32_t> missing;
                {
                    ScopedLockType sl(mCompleteLock);
                    missing = prevMissing(mCompleteLedgers,
                        mPubLedger->info().seq,
                        app_.getNodeStore().earliestSeq());
                }
                if (missing)
                {
                    JLOG(m_journal.trace()) <<
                        "tryAdvance discovered missing " << *missing;
                    if ((mFillInProgress == 0 || *missing > mFillInProgress) &&
                        shouldAcquire(mValidLedgerSeq, ledger_history_,
                            app_.getSHAMapStore().getCanDelete(), *missing))
                    {
                        JLOG(m_journal.trace()) <<
                            "advanceThread should acquire";
                    }
                    else
                        missing = boost::none;
                }
                if (! missing && mFillInProgress == 0)
                {
                    if (auto shardStore = app_.getShardStore())
                    {
                        missing = shardStore->prepareLedger(mValidLedgerSeq);
                        if (missing)
                            reason = InboundLedger::Reason::SHARD;
                    }
                }
                if(missing)
                {
                    fetchForHistory(*missing, progress, reason);
                    if (mValidLedgerSeq != mPubLedgerSeq)
                    {
                        JLOG (m_journal.debug()) <<
                            "tryAdvance found last valid changed";
                        progress = true;
                    }
                }
            }
            else
            {
                mHistLedger.reset();
                mShardLedger.reset();
                JLOG (m_journal.trace()) <<
                    "tryAdvance not fetching history";
            }
        }
        else
        {
            JLOG (m_journal.trace()) <<
                "tryAdvance found " << pubLedgers.size() <<
                " ledgers to publish";
            for(auto ledger : pubLedgers)
            {
                {
                    ScopedUnlockType sul (m_mutex);
                    JLOG (m_journal.debug()) <<
                        "tryAdvance publishing seq " << ledger->info().seq;
                    setFullLedger(ledger, true, true);
                }

                setPubLedger(ledger);

                {
                    ScopedUnlockType sul(m_mutex);
                    app_.getOPs().pubLedger(ledger);
                }
            }

            app_.getOPs().clearNeedNetworkLedger();
            progress = newPFWork ("pf:newLedger", sl);
        }
        if (progress)
            mAdvanceWork = true;
    } while (mAdvanceWork);
}

void
LedgerMaster::addFetchPack (
    uint256 const& hash,
    std::shared_ptr< Blob >& data)
{
    fetch_packs_.canonicalize (hash, data);
}

boost::optional<Blob>
LedgerMaster::getFetchPack (
    uint256 const& hash)
{
    Blob data;
    if (fetch_packs_.retrieve(hash, data))
    {
        fetch_packs_.del(hash, false);
        if (hash == sha512Half(makeSlice(data)))
            return data;
    }
    return boost::none;
}

void
LedgerMaster::gotFetchPack (
    bool progress,
    std::uint32_t seq)
{
    if (!mGotFetchPackThread.test_and_set(std::memory_order_acquire))
    {
        app_.getJobQueue().addJob (
            jtLEDGER_DATA, "gotFetchPack",
            [&] (Job&)
            {
                app_.getInboundLedgers().gotFetchPack();
                mGotFetchPackThread.clear(std::memory_order_release);
            });
    }
}

void
LedgerMaster::makeFetchPack (
    std::weak_ptr<Peer> const& wPeer,
    std::shared_ptr<protocol::TMGetObjectByHash> const& request,
    uint256 haveLedgerHash,
     UptimeClock::time_point uptime)
{
    if (UptimeClock::now() > uptime + 1s)
    {
        JLOG(m_journal.info()) << "Fetch pack request got stale";
        return;
    }

    if (app_.getFeeTrack ().isLoadedLocal () ||
        (getValidatedLedgerAge() > 40s))
    {
        JLOG(m_journal.info()) << "Too busy to make fetch pack";
        return;
    }

    auto peer = wPeer.lock ();

    if (!peer)
        return;

    auto haveLedger = getLedgerByHash (haveLedgerHash);

    if (!haveLedger)
    {
        JLOG(m_journal.info())
            << "Peer requests fetch pack for ledger we don't have: "
            << haveLedger;
        peer->charge (Resource::feeRequestNoReply);
        return;
    }

    if (haveLedger->open())
    {
        JLOG(m_journal.warn())
            << "Peer requests fetch pack from open ledger: "
            << haveLedger;
        peer->charge (Resource::feeInvalidRequest);
        return;
    }

    if (haveLedger->info().seq < getEarliestFetch())
    {
        JLOG(m_journal.debug())
            << "Peer requests fetch pack that is too early";
        peer->charge (Resource::feeInvalidRequest);
        return;
    }

    auto wantLedger = getLedgerByHash (haveLedger->info().parentHash);

    if (!wantLedger)
    {
        JLOG(m_journal.info())
            << "Peer requests fetch pack for ledger whose predecessor we "
            << "don't have: " << haveLedger;
        peer->charge (Resource::feeRequestNoReply);
        return;
    }


    auto fpAppender = [](
        protocol::TMGetObjectByHash* reply,
        std::uint32_t ledgerSeq,
        SHAMapHash const& hash,
        const Blob& blob)
    {
        protocol::TMIndexedObject& newObj = * (reply->add_objects ());
        newObj.set_ledgerseq (ledgerSeq);
        newObj.set_hash (hash.as_uint256().begin (), 256 / 8);
        newObj.set_data (&blob[0], blob.size ());
    };

    try
    {
        protocol::TMGetObjectByHash reply;
        reply.set_query (false);

        if (request->has_seq ())
            reply.set_seq (request->seq ());

        reply.set_ledgerhash (request->ledgerhash ());
        reply.set_type (protocol::TMGetObjectByHash::otFETCH_PACK);

        do
        {
            std::uint32_t lSeq = wantLedger->info().seq;

            protocol::TMIndexedObject& newObj = *reply.add_objects ();
            newObj.set_hash (
                wantLedger->info().hash.data(), 256 / 8);
            Serializer s (256);
            s.add32 (HashPrefix::ledgerMaster);
            addRaw(wantLedger->info(), s);
            newObj.set_data (s.getDataPtr (), s.getLength ());
            newObj.set_ledgerseq (lSeq);

            wantLedger->stateMap().getFetchPack
                (&haveLedger->stateMap(), true, 16384,
                    std::bind (fpAppender, &reply, lSeq, std::placeholders::_1,
                               std::placeholders::_2));

            if (wantLedger->info().txHash.isNonZero ())
                wantLedger->txMap().getFetchPack (
                    nullptr, true, 512,
                    std::bind (fpAppender, &reply, lSeq, std::placeholders::_1,
                               std::placeholders::_2));

            if (reply.objects ().size () >= 512)
                break;

            haveLedger = std::move (wantLedger);
            wantLedger = getLedgerByHash (haveLedger->info().parentHash);
        }
        while (wantLedger &&
               UptimeClock::now() <= uptime + 1s);

        JLOG(m_journal.info())
            << "Built fetch pack with " << reply.objects ().size () << " nodes";
        auto msg = std::make_shared<Message> (reply, protocol::mtGET_OBJECTS);
        peer->send (msg);
    }
    catch (std::exception const&)
    {
        JLOG(m_journal.warn()) << "Exception building fetch pach";
    }
}

std::size_t
LedgerMaster::getFetchPackCacheSize () const
{
    return fetch_packs_.getCacheSize ();
}

} 
























