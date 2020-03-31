

#ifndef RIPPLE_TXQ_H_INCLUDED
#define RIPPLE_TXQ_H_INCLUDED

#include <ripple/app/tx/applySteps.h>
#include <ripple/ledger/OpenView.h>
#include <ripple/ledger/ApplyView.h>
#include <ripple/protocol/TER.h>
#include <ripple/protocol/STTx.h>
#include <boost/intrusive/set.hpp>
#include <boost/circular_buffer.hpp>

namespace ripple {

class Application;
class Config;


class TxQ
{
public:
    static constexpr std::uint64_t baseLevel = 256;

    
    struct Setup
    {
        explicit Setup() = default;

        
        std::size_t ledgersInQueue = 20;
        
        std::size_t queueSizeMin = 2000;
        
        std::uint32_t retrySequencePercent = 25;
        
        std::int32_t multiTxnPercent = -90;
        std::uint64_t minimumEscalationMultiplier = baseLevel * 500;
        std::uint32_t minimumTxnInLedger = 5;
        std::uint32_t minimumTxnInLedgerSA = 1000;
        std::uint32_t targetTxnInLedger = 50;
        
        boost::optional<std::uint32_t> maximumTxnInLedger;
        
        std::uint32_t normalConsensusIncreasePercent = 20;
        
        std::uint32_t slowConsensusDecreasePercent = 50;
        std::uint32_t maximumTxnPerAccount = 10;
        
        std::uint32_t minimumLastLedgerBuffer = 2;
        
        std::uint64_t zeroBaseFeeTransactionFeeLevel = 256000;
        bool standAlone = false;
    };

    
    struct Metrics
    {
        explicit Metrics() = default;

        std::size_t txCount;
        boost::optional<std::size_t> txQMaxSize;
        std::size_t txInLedger;
        std::size_t txPerLedger;
        std::uint64_t referenceFeeLevel;
        std::uint64_t minProcessingFeeLevel;
        std::uint64_t medFeeLevel;
        std::uint64_t openLedgerFeeLevel;
    };

    
    struct AccountTxDetails
    {
        explicit AccountTxDetails() = default;

        uint64_t feeLevel;
        boost::optional<LedgerIndex const> lastValid;
        
        boost::optional<TxConsequences const> consequences;
    };

    
    struct TxDetails : AccountTxDetails
    {
        explicit TxDetails() = default;

        AccountID account;
        std::shared_ptr<STTx const> txn;
        
        int retriesRemaining;
        
        TER preflightResult;
        
        boost::optional<TER> lastResult;
    };

    TxQ(Setup const& setup,
        beast::Journal j);

    virtual ~TxQ();

    
    std::pair<TER, bool>
    apply(Application& app, OpenView& view,
        std::shared_ptr<STTx const> const& tx,
            ApplyFlags flags, beast::Journal j);

    
    bool
    accept(Application& app, OpenView& view);

    
    void
    processClosedLedger(Application& app,
        ReadView const& view, bool timeLeap);

    
    Metrics
    getMetrics(OpenView const& view) const;

    
    std::map<TxSeq, AccountTxDetails const>
    getAccountTxs(AccountID const& account, ReadView const& view) const;

    
    std::vector<TxDetails>
    getTxs(ReadView const& view) const;

    
    Json::Value
    doRPC(Application& app) const;

private:
    
    class FeeMetrics
    {
    private:
        std::size_t const minimumTxnCount_;
        std::size_t const targetTxnCount_;
        boost::optional<std::size_t> const maximumTxnCount_;
        std::size_t txnsExpected_;
        boost::circular_buffer<std::size_t> recentTxnCounts_;
        std::uint64_t escalationMultiplier_;
        beast::Journal j_;

    public:
        FeeMetrics(Setup const& setup, beast::Journal j)
            : minimumTxnCount_(setup.standAlone ?
                setup.minimumTxnInLedgerSA :
                setup.minimumTxnInLedger)
            , targetTxnCount_(setup.targetTxnInLedger < minimumTxnCount_ ?
                minimumTxnCount_ : setup.targetTxnInLedger)
            , maximumTxnCount_(setup.maximumTxnInLedger ?
                *setup.maximumTxnInLedger < targetTxnCount_ ?
                    targetTxnCount_ : *setup.maximumTxnInLedger :
                        boost::optional<std::size_t>(boost::none))
            , txnsExpected_(minimumTxnCount_)
            , recentTxnCounts_(setup.ledgersInQueue)
            , escalationMultiplier_(setup.minimumEscalationMultiplier)
            , j_(j)
        {
        }

        
        std::size_t
        update(Application& app,
            ReadView const& view, bool timeLeap,
            TxQ::Setup const& setup);

        struct Snapshot
        {
            std::size_t const txnsExpected;
            std::uint64_t const escalationMultiplier;
        };

        Snapshot
        getSnapshot() const
        {
            return {
                txnsExpected_,
                escalationMultiplier_
            };
        }

        
        static
        std::uint64_t
        scaleFeeLevel(Snapshot const& snapshot, OpenView const& view);

        
        static
        std::pair<bool, std::uint64_t>
        escalatedSeriesFeeLevel(Snapshot const& snapshot, OpenView const& view,
            std::size_t extraCount, std::size_t seriesSize);
    };

    
    class MaybeTx
    {
    public:
        boost::intrusive::set_member_hook<> byFeeListHook;

        std::shared_ptr<STTx const> txn;

        boost::optional<TxConsequences const> consequences;

        uint64_t const feeLevel;
        TxID const txID;
        boost::optional<TxID> priorTxID;
        AccountID const account;
        boost::optional<LedgerIndex> lastValid;
        TxSeq const sequence;
        
        int retriesRemaining;
        ApplyFlags const flags;
        
        boost::optional<TER> lastResult;
        
        boost::optional<PreflightResult const> pfresult;

        
        static constexpr int retriesAllowed = 10;

    public:
        MaybeTx(std::shared_ptr<STTx const> const&,
            TxID const& txID, std::uint64_t feeLevel,
                ApplyFlags const flags,
                    PreflightResult const& pfresult);

        std::pair<TER, bool>
        apply(Application& app, OpenView& view, beast::Journal j);
    };

    class GreaterFee
    {
    public:
        explicit GreaterFee() = default;

        bool operator()(const MaybeTx& lhs, const MaybeTx& rhs) const
        {
            return lhs.feeLevel > rhs.feeLevel;
        }
    };

    
    class TxQAccount
    {
    public:
        using TxMap = std::map <TxSeq, MaybeTx>;

        AccountID const account;
        TxMap transactions;
        
        bool retryPenalty = false;
        
        bool dropPenalty = false;

    public:
        explicit TxQAccount(std::shared_ptr<STTx const> const& txn);
        explicit TxQAccount(const AccountID& account);

        std::size_t
        getTxnCount() const
        {
            return transactions.size();
        }

        bool
        empty() const
        {
            return !getTxnCount();
        }

        MaybeTx&
        add(MaybeTx&&);

        
        bool
        remove(TxSeq const& sequence);
    };

    using FeeHook = boost::intrusive::member_hook
        <MaybeTx, boost::intrusive::set_member_hook<>,
        &MaybeTx::byFeeListHook>;

    using FeeMultiSet = boost::intrusive::multiset
        < MaybeTx, FeeHook,
        boost::intrusive::compare <GreaterFee> >;

    using AccountMap = std::map <AccountID, TxQAccount>;

    Setup const setup_;
    beast::Journal j_;

    
    FeeMetrics feeMetrics_;
    
    FeeMultiSet byFee_;
    
    AccountMap byAccount_;
    
    boost::optional<size_t> maxSize_;

    
    std::mutex mutable mutex_;

private:
    template<size_t fillPercentage = 100>
    bool
    isFull() const;

    
    bool canBeHeld(STTx const&, OpenView const&,
        AccountMap::iterator,
            boost::optional<FeeMultiSet::iterator>);

    FeeMultiSet::iterator_type erase(FeeMultiSet::const_iterator_type);
    
    FeeMultiSet::iterator_type eraseAndAdvance(FeeMultiSet::const_iterator_type);
    TxQAccount::TxMap::iterator
    erase(TxQAccount& txQAccount, TxQAccount::TxMap::const_iterator begin,
        TxQAccount::TxMap::const_iterator end);

    
    std::pair<TER, bool>
    tryClearAccountQueue(Application& app, OpenView& view,
        STTx const& tx, AccountMap::iterator const& accountIter,
            TxQAccount::TxMap::iterator, std::uint64_t feeLevelPaid,
                PreflightResult const& pfresult,
                    std::size_t const txExtraCount, ApplyFlags flags,
                        FeeMetrics::Snapshot const& metricsSnapshot,
                            beast::Journal j);

};


TxQ::Setup
setup_TxQ(Config const&);


std::unique_ptr<TxQ>
make_TxQ(TxQ::Setup const&, beast::Journal);

} 

#endif








