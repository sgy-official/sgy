

#include <ripple/app/ledger/LedgerCleaner.h>
#include <ripple/app/ledger/InboundLedgers.h>
#include <ripple/app/ledger/LedgerMaster.h>
#include <ripple/app/misc/LoadFeeTrack.h>
#include <ripple/protocol/jss.h>
#include <ripple/beast/core/CurrentThreadName.h>

namespace ripple {
namespace detail {



class LedgerCleanerImp : public LedgerCleaner
{
    Application& app_;
    beast::Journal j_;
    mutable std::mutex mutex_;

    mutable std::condition_variable wakeup_;

    std::thread thread_;

    enum class State : char {
        readyToClean = 0,
        startCleaning,
        cleaning
    };
    State state_ = State::readyToClean;
    bool shouldExit_ = false;

    LedgerIndex  minRange_ = 0;

    LedgerIndex  maxRange_ = 0;

    bool checkNodes_ = false;

    bool fixTxns_ = false;

    int failures_ = 0;

public:
    LedgerCleanerImp (
        Application& app,
        Stoppable& stoppable,
        beast::Journal journal)
        : LedgerCleaner (stoppable)
        , app_ (app)
        , j_ (journal)
    {
    }

    ~LedgerCleanerImp () override
    {
        if (thread_.joinable())
            LogicError ("LedgerCleanerImp::onStop not called.");
    }


    void onPrepare () override
    {
    }

    void onStart () override
    {
        thread_ = std::thread {&LedgerCleanerImp::run, this};
    }

    void onStop () override
    {
        JLOG (j_.info()) << "Stopping";
        {
            std::lock_guard<std::mutex> lock (mutex_);
            shouldExit_ = true;
            wakeup_.notify_one();
        }
        thread_.join();
    }


    void onWrite (beast::PropertyStream::Map& map) override
    {
        std::lock_guard<std::mutex> lock (mutex_);

        if (maxRange_ == 0)
            map["status"] = "idle";
        else
        {
            map["status"] = "running";
            map["min_ledger"] = minRange_;
            map["max_ledger"] = maxRange_;
            map["check_nodes"] = checkNodes_ ? "true" : "false";
            map["fix_txns"] = fixTxns_ ? "true" : "false";
            if (failures_ > 0)
                map["fail_counts"] = failures_;
        }
    }


    void doClean (Json::Value const& params) override
    {
        LedgerIndex minRange = 0;
        LedgerIndex maxRange = 0;
        app_.getLedgerMaster().getFullValidatedRange (minRange, maxRange);

        {
            std::lock_guard<std::mutex> lock (mutex_);

            maxRange_ = maxRange;
            minRange_ = minRange;
            checkNodes_ = false;
            fixTxns_ = false;
            failures_ = 0;

            

            if (params.isMember(jss::ledger))
            {
                maxRange_ = params[jss::ledger].asUInt();
                minRange_ = params[jss::ledger].asUInt();
                fixTxns_ = true;
                checkNodes_ = true;
            }

            if (params.isMember(jss::max_ledger))
                 maxRange_ = params[jss::max_ledger].asUInt();

            if (params.isMember(jss::min_ledger))
                minRange_ = params[jss::min_ledger].asUInt();

            if (params.isMember(jss::full))
                fixTxns_ = checkNodes_ = params[jss::full].asBool();

            if (params.isMember(jss::fix_txns))
                fixTxns_ = params[jss::fix_txns].asBool();

            if (params.isMember(jss::check_nodes))
                checkNodes_ = params[jss::check_nodes].asBool();

            if (params.isMember(jss::stop) && params[jss::stop].asBool())
                minRange_ = maxRange_ = 0;

            if (state_ == State::readyToClean)
            {
                state_ = State::startCleaning;
                wakeup_.notify_one();
            }
        }
    }

private:
    void init ()
    {
        JLOG (j_.debug()) << "Initializing";
    }

    void run ()
    {
        beast::setCurrentThreadName ("LedgerCleaner");
        JLOG (j_.debug()) << "Started";

        init();

        while (true)
        {
            {
                std::unique_lock<std::mutex> lock (mutex_);
                wakeup_.wait(lock, [this]()
                    {
                        return (
                            shouldExit_ ||
                            state_ == State::startCleaning);
                    });
                if (shouldExit_)
                    break;

                state_ = State::cleaning;
            }
            doLedgerCleaner();
        }

        stopped();
    }

    LedgerHash getLedgerHash(
        std::shared_ptr<ReadView const>& ledger,
        LedgerIndex index)
    {
        boost::optional<LedgerHash> hash;
        try
        {
            hash = hashOfSeq(*ledger, index, j_);
        }
        catch (SHAMapMissingNode &)
        {
            JLOG (j_.warn()) <<
                "Node missing from ledger " << ledger->info().seq;
            app_.getInboundLedgers().acquire (
                ledger->info().hash, ledger->info().seq,
                InboundLedger::Reason::GENERIC);
        }
        return hash ? *hash : beast::zero; 
    }

    
    bool doLedger(
        LedgerIndex const& ledgerIndex,
        LedgerHash const& ledgerHash,
        bool doNodes,
        bool doTxns)
    {
        auto nodeLedger = app_.getInboundLedgers().acquire (
            ledgerHash, ledgerIndex, InboundLedger::Reason::GENERIC);
        if (!nodeLedger)
        {
            JLOG (j_.debug()) << "Ledger " << ledgerIndex << " not available";
            app_.getLedgerMaster().clearLedger (ledgerIndex);
            app_.getInboundLedgers().acquire(
                ledgerHash, ledgerIndex, InboundLedger::Reason::GENERIC);
            return false;
        }

        auto dbLedger = loadByIndex(ledgerIndex, app_);
        if (! dbLedger ||
            (dbLedger->info().hash != ledgerHash) ||
            (dbLedger->info().parentHash != nodeLedger->info().parentHash))
        {
            JLOG (j_.debug()) <<
                "Ledger " << ledgerIndex << " mismatches SQL DB";
            doTxns = true;
        }

        if(! app_.getLedgerMaster().fixIndex(ledgerIndex, ledgerHash))
        {
            JLOG (j_.debug()) << "ledger " << ledgerIndex
                            << " had wrong entry in history";
            doTxns = true;
        }

        if (doNodes && !nodeLedger->walkLedger(app_.journal ("Ledger")))
        {
            JLOG (j_.debug()) << "Ledger " << ledgerIndex << " is missing nodes";
            app_.getLedgerMaster().clearLedger (ledgerIndex);
            app_.getInboundLedgers().acquire(
                ledgerHash, ledgerIndex, InboundLedger::Reason::GENERIC);
            return false;
        }

        if (doTxns && !pendSaveValidated(app_, nodeLedger, true, false))
        {
            JLOG (j_.debug()) << "Failed to save ledger " << ledgerIndex;
            return false;
        }

        return true;
    }

    
    LedgerHash getHash(
        LedgerIndex const& ledgerIndex,
        std::shared_ptr<ReadView const>& referenceLedger)
    {
        LedgerHash ledgerHash;

        if (!referenceLedger || (referenceLedger->info().seq < ledgerIndex))
        {
            referenceLedger = app_.getLedgerMaster().getValidatedLedger();
            if (!referenceLedger)
            {
                JLOG (j_.warn()) << "No validated ledger";
                return ledgerHash; 
            }
        }

        if (referenceLedger->info().seq >= ledgerIndex)
        {
            ledgerHash = getLedgerHash(referenceLedger, ledgerIndex);
            if (ledgerHash.isZero())
            {
                LedgerIndex refIndex = getCandidateLedger (ledgerIndex);
                LedgerHash refHash = getLedgerHash (referenceLedger, refIndex);

                bool const nonzero (refHash.isNonZero ());
                assert (nonzero);
                if (nonzero)
                {
                    referenceLedger =
                        app_.getInboundLedgers().acquire(
                            refHash, refIndex, InboundLedger::Reason::GENERIC);
                    if (referenceLedger)
                        ledgerHash = getLedgerHash(
                            referenceLedger, ledgerIndex);
                }
            }
        }
        else
            JLOG (j_.warn()) << "Validated ledger is prior to target ledger";

        return ledgerHash;
    }

    
    void doLedgerCleaner()
    {
        auto shouldExit = [this]()
        {
            std::lock_guard<std::mutex> lock(mutex_);
            return shouldExit_;
        };

        std::shared_ptr<ReadView const> goodLedger;

        while (! shouldExit())
        {
            LedgerIndex ledgerIndex;
            LedgerHash ledgerHash;
            bool doNodes;
            bool doTxns;

            while (app_.getFeeTrack().isLoadedLocal())
            {
                JLOG (j_.debug()) << "Waiting for load to subside";
                std::this_thread::sleep_for(std::chrono::seconds(5));
                if (shouldExit())
                    return;
            }

            {
                std::lock_guard<std::mutex> lock (mutex_);
                if ((minRange_ > maxRange_) ||
                    (maxRange_ == 0) || (minRange_ == 0))
                {
                    minRange_ = maxRange_ = 0;
                    state_ = State::readyToClean;
                    return;
                }
                ledgerIndex = maxRange_;
                doNodes = checkNodes_;
                doTxns = fixTxns_;
            }

            ledgerHash = getHash(ledgerIndex, goodLedger);

            bool fail = false;
            if (ledgerHash.isZero())
            {
                JLOG (j_.info()) << "Unable to get hash for ledger "
                               << ledgerIndex;
                fail = true;
            }
            else if (!doLedger(ledgerIndex, ledgerHash, doNodes, doTxns))
            {
                JLOG (j_.info()) << "Failed to process ledger " << ledgerIndex;
                fail = true;
            }

            if (fail)
            {
                {
                    std::lock_guard<std::mutex> lock (mutex_);
                    ++failures_;
                }
                std::this_thread::sleep_for(std::chrono::seconds(2));
            }
            else
            {
                {
                    std::lock_guard<std::mutex> lock (mutex_);
                    if (ledgerIndex == minRange_)
                        ++minRange_;
                    if (ledgerIndex == maxRange_)
                        --maxRange_;
                    failures_ = 0;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

        }
    }
};


LedgerCleaner::LedgerCleaner (Stoppable& parent)
    : Stoppable ("LedgerCleaner", parent)
    , beast::PropertyStream::Source ("ledgercleaner")
{
}

LedgerCleaner::~LedgerCleaner() = default;

std::unique_ptr<LedgerCleaner>
make_LedgerCleaner (Application& app,
    Stoppable& parent, beast::Journal journal)
{
    return std::make_unique<LedgerCleanerImp>(app, parent, journal);
}

} 
} 
























