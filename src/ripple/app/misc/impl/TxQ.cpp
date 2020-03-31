

#include <ripple/app/misc/TxQ.h>
#include <ripple/app/ledger/OpenLedger.h>
#include <ripple/app/main/Application.h>
#include <ripple/app/misc/LoadFeeTrack.h>
#include <ripple/app/tx/apply.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/jss.h>
#include <ripple/protocol/st.h>
#include <ripple/basics/mulDiv.h>
#include <boost/algorithm/clamp.hpp>
#include <limits>
#include <numeric>
#include <algorithm>

namespace ripple {


static
std::uint64_t
getFeeLevelPaid(
    STTx const& tx,
    std::uint64_t baseRefLevel,
    std::uint64_t refTxnCostDrops,
    TxQ::Setup const& setup)
{
    if (refTxnCostDrops == 0)
        return setup.zeroBaseFeeTransactionFeeLevel;

    return mulDiv(tx[sfFee].xrp().drops(),
        baseRefLevel,
            refTxnCostDrops).second;
}

static
boost::optional<LedgerIndex>
getLastLedgerSequence(STTx const& tx)
{
    if (!tx.isFieldPresent(sfLastLedgerSequence))
        return boost::none;
    return tx.getFieldU32(sfLastLedgerSequence);
}

static
std::uint64_t
increase(std::uint64_t level,
    std::uint32_t increasePercent)
{
    return mulDiv(
        level, 100 + increasePercent, 100).second;
}



std::size_t
TxQ::FeeMetrics::update(Application& app,
    ReadView const& view, bool timeLeap,
    TxQ::Setup const& setup)
{
    std::vector<uint64_t> feeLevels;
    auto const txBegin = view.txs.begin();
    auto const txEnd = view.txs.end();
    auto const size = std::distance(txBegin, txEnd);
    feeLevels.reserve(size);
    std::for_each(txBegin, txEnd,
        [&](auto const& tx)
        {
            auto const baseFee = calculateBaseFee(view, *tx.first);
            feeLevels.push_back(getFeeLevelPaid(*tx.first,
                baseLevel, baseFee, setup));
        }
    );
    std::sort(feeLevels.begin(), feeLevels.end());
    assert(size == feeLevels.size());

    JLOG(j_.debug()) << "Ledger " << view.info().seq <<
        " has " << size << " transactions. " <<
        "Ledgers are processing " <<
        (timeLeap ? "slowly" : "as expected") <<
        ". Expected transactions is currently " <<
        txnsExpected_ << " and multiplier is " <<
        escalationMultiplier_;

    if (timeLeap)
    {
        auto const cutPct = 100 - setup.slowConsensusDecreasePercent;
        auto const upperLimit = std::max<std::uint64_t>(
            mulDiv(txnsExpected_, cutPct, 100).second, minimumTxnCount_);
        txnsExpected_ = boost::algorithm::clamp(
            mulDiv(size, cutPct, 100).second, minimumTxnCount_, upperLimit);
        recentTxnCounts_.clear();
    }
    else if (size > txnsExpected_ ||
        size > targetTxnCount_)
    {
        recentTxnCounts_.push_back(
            mulDiv(size, 100 + setup.normalConsensusIncreasePercent,
                100).second);
        auto const iter = std::max_element(recentTxnCounts_.begin(),
            recentTxnCounts_.end());
        BOOST_ASSERT(iter != recentTxnCounts_.end());
        auto const next = [&]
        {
            if (*iter >= txnsExpected_)
                return *iter;
            return (txnsExpected_ * 9 + *iter) / 10;
        }();
        txnsExpected_ = std::min(next,
            maximumTxnCount_.value_or(next));
    }

    if (!size)
    {
        escalationMultiplier_ = setup.minimumEscalationMultiplier;
    }
    else
    {
        escalationMultiplier_ = (feeLevels[size / 2] +
            feeLevels[(size - 1) / 2] + 1) / 2;
        escalationMultiplier_ = std::max(escalationMultiplier_,
            setup.minimumEscalationMultiplier);
    }
    JLOG(j_.debug()) << "Expected transactions updated to " <<
        txnsExpected_ << " and multiplier updated to " <<
        escalationMultiplier_;

    return size;
}

std::uint64_t
TxQ::FeeMetrics::scaleFeeLevel(Snapshot const& snapshot,
    OpenView const& view)
{
    auto const current = view.txCount();

    auto const target = snapshot.txnsExpected;
    auto const multiplier = snapshot.escalationMultiplier;

    if (current > target)
    {
        return mulDiv(multiplier, current * current,
            target * target).second;
    }

    return baseLevel;
}

namespace detail {

static
std::pair<bool, std::uint64_t>
sumOfFirstSquares(std::size_t x)
{

    if (x >= (1 << 21))
        return std::make_pair(false,
            std::numeric_limits<std::uint64_t>::max());
    return std::make_pair(true, (x * (x + 1) * (2 * x + 1)) / 6);
}

}

std::pair<bool, std::uint64_t>
TxQ::FeeMetrics::escalatedSeriesFeeLevel(Snapshot const& snapshot,
    OpenView const& view, std::size_t extraCount,
    std::size_t seriesSize)
{
    
    auto const current = view.txCount() + extraCount;
    
    auto const last = current + seriesSize - 1;

    auto const target = snapshot.txnsExpected;
    auto const multiplier = snapshot.escalationMultiplier;

    assert(current > target);

    
    auto const sumNlast = detail::sumOfFirstSquares(last);
    auto const sumNcurrent = detail::sumOfFirstSquares(current - 1);
    if (!sumNlast.first)
        return sumNlast;
    auto const totalFeeLevel = mulDiv(multiplier,
        sumNlast.second - sumNcurrent.second, target * target);

    return totalFeeLevel;
}


TxQ::MaybeTx::MaybeTx(
    std::shared_ptr<STTx const> const& txn_,
        TxID const& txID_, std::uint64_t feeLevel_,
            ApplyFlags const flags_,
                PreflightResult const& pfresult_)
    : txn(txn_)
    , feeLevel(feeLevel_)
    , txID(txID_)
    , account(txn_->getAccountID(sfAccount))
    , sequence(txn_->getSequence())
    , retriesRemaining(retriesAllowed)
    , flags(flags_)
    , pfresult(pfresult_)
{
    lastValid = getLastLedgerSequence(*txn);

    if (txn->isFieldPresent(sfAccountTxnID))
        priorTxID = txn->getFieldH256(sfAccountTxnID);
}

std::pair<TER, bool>
TxQ::MaybeTx::apply(Application& app, OpenView& view, beast::Journal j)
{
    boost::optional<STAmountSO> saved;
    if (view.rules().enabled(fix1513))
        saved.emplace(view.info().parentCloseTime);
    assert(pfresult);
    if (pfresult->rules != view.rules() ||
        pfresult->flags != flags)
    {
        JLOG(j.debug()) << "Queued transaction " <<
            txID << " rules or flags have changed. Flags from " <<
            pfresult->flags << " to " << flags ;

        pfresult.emplace(
            preflight(app, view.rules(),
                pfresult->tx,
                flags,
                pfresult->j));
    }

    auto pcresult = preclaim(
        *pfresult, app, view);

    return doApply(pcresult, app, view);
}

TxQ::TxQAccount::TxQAccount(std::shared_ptr<STTx const> const& txn)
    :TxQAccount(txn->getAccountID(sfAccount))
{
}

TxQ::TxQAccount::TxQAccount(const AccountID& account_)
    : account(account_)
{
}

auto
TxQ::TxQAccount::add(MaybeTx&& txn)
    -> MaybeTx&
{
    auto sequence = txn.sequence;

    auto result = transactions.emplace(sequence, std::move(txn));
    assert(result.second);
    assert(&result.first->second != &txn);

    return result.first->second;
}

bool
TxQ::TxQAccount::remove(TxSeq const& sequence)
{
    return transactions.erase(sequence) != 0;
}


TxQ::TxQ(Setup const& setup,
    beast::Journal j)
    : setup_(setup)
    , j_(j)
    , feeMetrics_(setup, j)
    , maxSize_(boost::none)
{
}

TxQ::~TxQ()
{
    byFee_.clear();
}

template<size_t fillPercentage>
bool
TxQ::isFull() const
{
    static_assert(fillPercentage > 0 &&
        fillPercentage <= 100,
            "Invalid fill percentage");
    return maxSize_ && byFee_.size() >=
        (*maxSize_ * fillPercentage / 100);
}

bool
TxQ::canBeHeld(STTx const& tx, OpenView const& view,
    AccountMap::iterator accountIter,
        boost::optional<FeeMultiSet::iterator> replacementIter)
{
    bool canBeHeld =
        ! tx.isFieldPresent(sfPreviousTxnID) &&
        ! tx.isFieldPresent(sfAccountTxnID);
    if (canBeHeld)
    {
        
        auto const lastValid = getLastLedgerSequence(tx);
        canBeHeld = !lastValid || *lastValid >=
            view.info().seq + setup_.minimumLastLedgerBuffer;
    }
    if (canBeHeld)
    {
        

        canBeHeld = accountIter == byAccount_.end();

        if(!canBeHeld)
        {
            canBeHeld = replacementIter.is_initialized();
        }

        if (!canBeHeld)
        {
            canBeHeld = accountIter->second.getTxnCount() <
                setup_.maximumTxnPerAccount;
        }

        if (!canBeHeld)
        {
            auto const tSeq = tx.getSequence();
            canBeHeld = tSeq < accountIter->second.transactions.rbegin()->first;
        }
    }
    return canBeHeld;
}

auto
TxQ::erase(TxQ::FeeMultiSet::const_iterator_type candidateIter)
    -> FeeMultiSet::iterator_type
{
    auto& txQAccount = byAccount_.at(candidateIter->account);
    auto const sequence = candidateIter->sequence;
    auto const newCandidateIter = byFee_.erase(candidateIter);
    auto const found = txQAccount.remove(sequence);
    (void)found;
    assert(found);

    return newCandidateIter;
}

auto
TxQ::eraseAndAdvance(TxQ::FeeMultiSet::const_iterator_type candidateIter)
    -> FeeMultiSet::iterator_type
{
    auto& txQAccount = byAccount_.at(candidateIter->account);
    auto const accountIter = txQAccount.transactions.find(
        candidateIter->sequence);
    assert(accountIter != txQAccount.transactions.end());
    assert(accountIter == txQAccount.transactions.begin());
    assert(byFee_.iterator_to(accountIter->second) == candidateIter);
    auto const accountNextIter = std::next(accountIter);
    
    auto const feeNextIter = std::next(candidateIter);
    bool const useAccountNext = accountNextIter != txQAccount.transactions.end() &&
        accountNextIter->first == candidateIter->sequence + 1 &&
            (feeNextIter == byFee_.end() ||
                accountNextIter->second.feeLevel > feeNextIter->feeLevel);
    auto const candidateNextIter = byFee_.erase(candidateIter);
    txQAccount.transactions.erase(accountIter);
    return useAccountNext ?
        byFee_.iterator_to(accountNextIter->second) :
            candidateNextIter;

}

auto
TxQ::erase(TxQ::TxQAccount& txQAccount,
    TxQ::TxQAccount::TxMap::const_iterator begin,
        TxQ::TxQAccount::TxMap::const_iterator end)
            -> TxQAccount::TxMap::iterator
{
    for (auto it = begin; it != end; ++it)
    {
        byFee_.erase(byFee_.iterator_to(it->second));
    }
    return txQAccount.transactions.erase(begin, end);
}

std::pair<TER, bool>
TxQ::tryClearAccountQueue(Application& app, OpenView& view,
    STTx const& tx, TxQ::AccountMap::iterator const& accountIter,
        TxQAccount::TxMap::iterator beginTxIter, std::uint64_t feeLevelPaid,
            PreflightResult const& pfresult, std::size_t const txExtraCount,
                ApplyFlags flags, FeeMetrics::Snapshot const& metricsSnapshot,
                    beast::Journal j)
{
    auto const tSeq = tx.getSequence();
    assert(beginTxIter != accountIter->second.transactions.end());
    auto const aSeq = beginTxIter->first;

    auto const requiredTotalFeeLevel = FeeMetrics::escalatedSeriesFeeLevel(
        metricsSnapshot, view, txExtraCount, tSeq - aSeq + 1);
    
    if (!requiredTotalFeeLevel.first)
        return std::make_pair(telINSUF_FEE_P, false);

    auto endTxIter = accountIter->second.transactions.lower_bound(tSeq);

    auto const totalFeeLevelPaid = std::accumulate(beginTxIter, endTxIter,
        feeLevelPaid,
        [](auto const& total, auto const& tx)
        {
            return total + tx.second.feeLevel;
        });

    if (totalFeeLevelPaid < requiredTotalFeeLevel.second)
        return std::make_pair(telINSUF_FEE_P, false);

    for (auto it = beginTxIter; it != endTxIter; ++it)
    {
        auto txResult = it->second.apply(app, view, j);
        --it->second.retriesRemaining;
        it->second.lastResult = txResult.first;
        if (!txResult.second)
        {
            return std::make_pair(txResult.first, false);
        }
    }
    auto txResult = [&]{
        boost::optional<STAmountSO> saved;
        if (view.rules().enabled(fix1513))
            saved.emplace(view.info().parentCloseTime);
        auto const pcresult = preclaim(pfresult, app, view);
        return doApply(pcresult, app, view);
    }();

    if (txResult.second)
    {
        endTxIter = erase(accountIter->second, beginTxIter, endTxIter);
        if (endTxIter != accountIter->second.transactions.end() &&
            endTxIter->first == tSeq)
            erase(accountIter->second, endTxIter, std::next(endTxIter));
    }

    return txResult;
}



std::pair<TER, bool>
TxQ::apply(Application& app, OpenView& view,
    std::shared_ptr<STTx const> const& tx,
        ApplyFlags flags, beast::Journal j)
{
    auto const account = (*tx)[sfAccount];
    auto const transactionID = tx->getTransactionID();
    auto const tSeq = tx->getSequence();

    boost::optional<STAmountSO> saved;
    if (view.rules().enabled(fix1513))
        saved.emplace(view.info().parentCloseTime);

    auto const pfresult = preflight(app, view.rules(),
            *tx, flags, j);
    if (pfresult.ter != tesSUCCESS)
        return{ pfresult.ter, false };

    struct MultiTxn
    {
        explicit MultiTxn() = default;

        boost::optional<ApplyViewImpl> applyView;
        boost::optional<OpenView> openView;

        TxQAccount::TxMap::iterator nextTxIter;

        XRPAmount fee = beast::zero;
        XRPAmount potentialSpend = beast::zero;
        bool includeCurrentFee = false;
    };

    boost::optional<MultiTxn> multiTxn;
    boost::optional<TxConsequences const> consequences;
    boost::optional<FeeMultiSet::iterator> replacedItemDeleteIter;

    std::lock_guard<std::mutex> lock(mutex_);

    auto const metricsSnapshot = feeMetrics_.getSnapshot();

    auto const baseFee = calculateBaseFee(view, *tx);
    auto const feeLevelPaid = getFeeLevelPaid(*tx,
        baseLevel, baseFee, setup_);
    auto const requiredFeeLevel = [&]()
    {
        auto feeLevel = FeeMetrics::scaleFeeLevel(metricsSnapshot, view);
        if ((flags & tapPREFER_QUEUE) && byFee_.size())
        {
            return std::max(feeLevel, byFee_.begin()->feeLevel);
        }
        return feeLevel;
    }();

    auto accountIter = byAccount_.find(account);
    bool const accountExists = accountIter != byAccount_.end();

    if (accountExists)
    {
        auto& txQAcct = accountIter->second;
        auto existingIter = txQAcct.transactions.find(tSeq);
        if (existingIter != txQAcct.transactions.end())
        {
            auto requiredRetryLevel = increase(
                existingIter->second.feeLevel,
                    setup_.retrySequencePercent);
            JLOG(j_.trace()) << "Found transaction in queue for account " <<
                account << " with sequence number " << tSeq <<
                " new txn fee level is " << feeLevelPaid <<
                ", old txn fee level is " <<
                existingIter->second.feeLevel <<
                ", new txn needs fee level of " <<
                requiredRetryLevel;
            if (feeLevelPaid > requiredRetryLevel
                || (existingIter->second.feeLevel < requiredFeeLevel &&
                    feeLevelPaid >= requiredFeeLevel &&
                    existingIter == txQAcct.transactions.begin()))
            {
                

                
                if (std::next(existingIter) != txQAcct.transactions.end())
                {
                    if (!existingIter->second.consequences)
                        existingIter->second.consequences.emplace(
                            calculateConsequences(
                                *existingIter->second.pfresult));

                    if (existingIter->second.consequences->category ==
                        TxConsequences::normal)
                    {
                        assert(!consequences);
                        consequences.emplace(calculateConsequences(
                            pfresult));
                        if (consequences->category ==
                            TxConsequences::blocker)
                        {
                            JLOG(j_.trace()) <<
                                "Ignoring blocker transaction " <<
                                transactionID <<
                                " in favor of normal queued " <<
                                existingIter->second.txID;
                            return {telCAN_NOT_QUEUE_BLOCKS, false };
                        }
                    }
                }


                JLOG(j_.trace()) <<
                    "Removing transaction from queue " <<
                    existingIter->second.txID <<
                    " in favor of " << transactionID;
                auto deleteIter = byFee_.iterator_to(existingIter->second);
                assert(deleteIter != byFee_.end());
                assert(&existingIter->second == &*deleteIter);
                assert(deleteIter->sequence == tSeq);
                assert(deleteIter->account == txQAcct.account);
                replacedItemDeleteIter = deleteIter;
            }
            else
            {
                JLOG(j_.trace()) <<
                    "Ignoring transaction " <<
                    transactionID <<
                    " in favor of queued " <<
                    existingIter->second.txID;
                return{ telCAN_NOT_QUEUE_FEE, false };
            }
        }
    }

    if (accountExists)
    {
        auto const sle = view.read(keylet::account(account));

        if (sle)
        {
            auto& txQAcct = accountIter->second;
            auto const aSeq = (*sle)[sfSequence];

            if (aSeq < tSeq)
            {
                if (canBeHeld(*tx, view, accountIter, replacedItemDeleteIter))
                    multiTxn.emplace();
            }

            if (multiTxn)
            {
                
                multiTxn->nextTxIter = txQAcct.transactions.find(aSeq);
                auto workingIter = multiTxn->nextTxIter;
                auto workingSeq = aSeq;
                for (; workingIter != txQAcct.transactions.end();
                    ++workingIter, ++workingSeq)
                {
                    if (workingSeq < tSeq &&
                        workingIter->first != workingSeq)
                    {
                        multiTxn.reset();
                        break;
                    }
                    if (workingIter->first == tSeq - 1)
                    {
                        auto requiredMultiLevel = increase(
                            workingIter->second.feeLevel,
                            setup_.multiTxnPercent);

                        if (feeLevelPaid <= requiredMultiLevel)
                        {
                            JLOG(j_.trace()) <<
                                "Ignoring transaction " <<
                                transactionID <<
                                ". Needs fee level of " <<

                                requiredMultiLevel <<
                                ". Only paid " <<
                                feeLevelPaid;
                            return{ telINSUF_FEE_P, false };
                        }
                    }
                    if (workingIter->first == tSeq)
                    {
                        assert(replacedItemDeleteIter);
                        multiTxn->includeCurrentFee =
                            std::next(workingIter) !=
                                txQAcct.transactions.end();
                        continue;
                    }
                    if (!workingIter->second.consequences)
                        workingIter->second.consequences.emplace(
                            calculateConsequences(
                                *workingIter->second.pfresult));
                    if (workingIter->first < tSeq &&
                        workingIter->second.consequences->category ==
                            TxConsequences::blocker)
                    {
                        JLOG(j_.trace()) <<
                            "Ignoring transaction " <<
                            transactionID <<
                            ". A blocker-type transaction " <<
                            "is in the queue.";
                        return{ telCAN_NOT_QUEUE_BLOCKED, false };
                    }
                    multiTxn->fee +=
                        workingIter->second.consequences->fee;
                    multiTxn->potentialSpend +=
                        workingIter->second.consequences->potentialSpend;
                }
                if (workingSeq < tSeq)
                    multiTxn.reset();
            }

            if (multiTxn)
            {
                
                auto const balance = (*sle)[sfBalance].xrp();
                
                auto const reserve = view.fees().accountReserve(0);
                auto totalFee = multiTxn->fee;
                if (multiTxn->includeCurrentFee)
                    totalFee += (*tx)[sfFee].xrp();
                if (totalFee >= balance ||
                    totalFee >= reserve)
                {
                    JLOG(j_.trace()) <<
                        "Ignoring transaction " <<
                        transactionID <<
                        ". Total fees in flight too high.";
                    return{ telCAN_NOT_QUEUE_BALANCE, false };
                }

                multiTxn->applyView.emplace(&view, flags);
                multiTxn->openView.emplace(&*multiTxn->applyView);

                auto const sleBump = multiTxn->applyView->peek(
                    keylet::account(account));

                auto const potentialTotalSpend = multiTxn->fee +
                    std::min(balance - std::min(balance, reserve),
                    multiTxn->potentialSpend);
                assert(potentialTotalSpend > 0);
                sleBump->setFieldAmount(sfBalance,
                    balance - potentialTotalSpend);
                sleBump->setFieldU32(sfSequence, tSeq);
            }
        }
    }

    assert(!multiTxn || multiTxn->openView);
    auto const pcresult = preclaim(pfresult, app,
            multiTxn ? *multiTxn->openView : view);
    if (!pcresult.likelyToClaimFee)
        return{ pcresult.ter, false };

    assert(feeLevelPaid >= baseLevel);

    JLOG(j_.trace()) << "Transaction " <<
        transactionID <<
        " from account " << account <<
        " has fee level of " << feeLevelPaid <<
        " needs at least " << requiredFeeLevel <<
        " to get in the open ledger, which has " <<
        view.txCount() << " entries.";

    
    if (!(flags & tapPREFER_QUEUE) && accountExists &&
        multiTxn.is_initialized() &&
        multiTxn->nextTxIter->second.retriesRemaining ==
            MaybeTx::retriesAllowed &&
                feeLevelPaid > requiredFeeLevel &&
                    requiredFeeLevel > baseLevel && baseFee != 0)
    {
        OpenView sandbox(open_ledger, &view, view.rules());

        auto result = tryClearAccountQueue(app, sandbox, *tx, accountIter,
            multiTxn->nextTxIter, feeLevelPaid, pfresult, view.txCount(),
                flags, metricsSnapshot, j);
        if (result.second)
        {
            sandbox.apply(view);
            
            return result;
        }
    }

    if (!multiTxn && feeLevelPaid >= requiredFeeLevel)
    {

        JLOG(j_.trace()) << "Applying transaction " <<
            transactionID <<
            " to open ledger.";
        ripple::TER txnResult;
        bool didApply;

        std::tie(txnResult, didApply) = doApply(pcresult, app, view);

        JLOG(j_.trace()) << "New transaction " <<
            transactionID <<
                (didApply ? " applied successfully with " :
                    " failed with ") <<
                        transToken(txnResult);

        if (didApply && replacedItemDeleteIter)
            erase(*replacedItemDeleteIter);
        return { txnResult, didApply };
    }

    if (! multiTxn &&
        ! canBeHeld(*tx, view, accountIter, replacedItemDeleteIter))
    {
        JLOG(j_.trace()) << "Transaction " <<
            transactionID <<
            " can not be held";
        return { telCAN_NOT_QUEUE, false };
    }

    if (!replacedItemDeleteIter && isFull())
    {
        auto lastRIter = byFee_.rbegin();
        if (lastRIter->account == account)
        {
            JLOG(j_.warn()) << "Queue is full, and transaction " <<
                transactionID <<
                " would kick a transaction from the same account (" <<
                account << ") out of the queue.";
            return { telCAN_NOT_QUEUE_FULL, false };
        }
        auto const& endAccount = byAccount_.at(lastRIter->account);
        auto endEffectiveFeeLevel = [&]()
        {
            if (lastRIter->feeLevel > feeLevelPaid
                || endAccount.transactions.size() == 1)
                return lastRIter->feeLevel;

            constexpr std::uint64_t max =
                std::numeric_limits<std::uint64_t>::max();
            auto endTotal = std::accumulate(endAccount.transactions.begin(),
                endAccount.transactions.end(),
                    std::pair<std::uint64_t, std::uint64_t>(0, 0),
                        [&](auto const& total, auto const& tx)
                        {
                            auto next = tx.second.feeLevel /
                                endAccount.transactions.size();
                            auto mod = tx.second.feeLevel %
                                endAccount.transactions.size();
                            if (total.first >= max - next ||
                                    total.second >= max - mod)
                                return std::make_pair(max, std::uint64_t(0));
                            return std::make_pair(total.first + next, total.second + mod);
                        });
            return endTotal.first + endTotal.second /
                endAccount.transactions.size();
        }();
        if (feeLevelPaid > endEffectiveFeeLevel)
        {
            auto dropRIter = endAccount.transactions.rbegin();
            assert(dropRIter->second.account == lastRIter->account);
            JLOG(j_.warn()) <<
                "Removing last item of account " <<
                lastRIter->account <<
                " from queue with average fee of " <<
                endEffectiveFeeLevel << " in favor of " <<
                transactionID << " with fee of " <<
                feeLevelPaid;
            erase(byFee_.iterator_to(dropRIter->second));
        }
        else
        {
            JLOG(j_.warn()) << "Queue is full, and transaction " <<
                transactionID <<
                " fee is lower than end item's account average fee";
            return { telCAN_NOT_QUEUE_FULL, false };
        }
    }

    if (replacedItemDeleteIter)
        erase(*replacedItemDeleteIter);
    if (!accountExists)
    {
        bool created;
        std::tie(accountIter, created) = byAccount_.emplace(
            account, TxQAccount(tx));
        (void)created;
        assert(created);
    }

    flags &= ~tapRETRY;

    flags &= ~tapPREFER_QUEUE;

    auto& candidate = accountIter->second.add(
        { tx, transactionID, feeLevelPaid, flags, pfresult });
    
    if (consequences)
        candidate.consequences.emplace(*consequences);
    byFee_.insert(candidate);
    JLOG(j_.debug()) << "Added transaction " << candidate.txID <<
        " with result " << transToken(pfresult.ter) <<
        " from " << (accountExists ? "existing" : "new") <<
            " account " << candidate.account << " to queue." <<
            " Flags: " << flags;

    return { terQUEUED, false };
}


void
TxQ::processClosedLedger(Application& app,
    ReadView const& view, bool timeLeap)
{
    std::lock_guard<std::mutex> lock(mutex_);

    feeMetrics_.update(app, view, timeLeap, setup_);
    auto const& snapshot = feeMetrics_.getSnapshot();

    auto ledgerSeq = view.info().seq;

    if (!timeLeap)
        maxSize_ = std::max (snapshot.txnsExpected * setup_.ledgersInQueue,
             setup_.queueSizeMin);

    for(auto candidateIter = byFee_.begin(); candidateIter != byFee_.end(); )
    {
        if (candidateIter->lastValid
            && *candidateIter->lastValid <= ledgerSeq)
        {
            byAccount_.at(candidateIter->account).dropPenalty = true;
            candidateIter = erase(candidateIter);
        }
        else
        {
            ++candidateIter;
        }
    }

    for (auto txQAccountIter = byAccount_.begin();
        txQAccountIter != byAccount_.end();)
    {
        if (txQAccountIter->second.empty())
            txQAccountIter = byAccount_.erase(txQAccountIter);
        else
            ++txQAccountIter;
    }
}


bool
TxQ::accept(Application& app,
    OpenView& view)
{
    

    auto ledgerChanged = false;

    std::lock_guard<std::mutex> lock(mutex_);

    auto const metricSnapshot = feeMetrics_.getSnapshot();

    for (auto candidateIter = byFee_.begin(); candidateIter != byFee_.end();)
    {
        auto& account = byAccount_.at(candidateIter->account);
        if (candidateIter->sequence >
            account.transactions.begin()->first)
        {
            JLOG(j_.trace()) << "Skipping queued transaction " <<
                candidateIter->txID << " from account " <<
                candidateIter->account << " as it is not the first.";
            candidateIter++;
            continue;
        }
        auto const requiredFeeLevel = FeeMetrics::scaleFeeLevel(
            metricSnapshot, view);
        auto const feeLevelPaid = candidateIter->feeLevel;
        JLOG(j_.trace()) << "Queued transaction " <<
            candidateIter->txID << " from account " <<
            candidateIter->account << " has fee level of " <<
            feeLevelPaid << " needs at least " <<
            requiredFeeLevel;
        if (feeLevelPaid >= requiredFeeLevel)
        {
            auto firstTxn = candidateIter->txn;

            JLOG(j_.trace()) << "Applying queued transaction " <<
                candidateIter->txID << " to open ledger.";

            TER txnResult;
            bool didApply;
            std::tie(txnResult, didApply) = candidateIter->apply(app, view, j_);

            if (didApply)
            {
                JLOG(j_.debug()) << "Queued transaction " <<
                    candidateIter->txID <<
                    " applied successfully with " <<
                    transToken(txnResult) << ". Remove from queue.";

                candidateIter = eraseAndAdvance(candidateIter);
                ledgerChanged = true;
            }
            else if (isTefFailure(txnResult) || isTemMalformed(txnResult) ||
                candidateIter->retriesRemaining <= 0)
            {
                if (candidateIter->retriesRemaining <= 0)
                    account.retryPenalty = true;
                else
                    account.dropPenalty = true;
                JLOG(j_.debug()) << "Queued transaction " <<
                    candidateIter->txID << " failed with " <<
                    transToken(txnResult) << ". Remove from queue.";
                candidateIter = eraseAndAdvance(candidateIter);
            }
            else
            {
                JLOG(j_.debug()) << "Queued transaction " <<
                    candidateIter->txID << " failed with " <<
                    transToken(txnResult) << ". Leave in queue." <<
                    " Applied: " << didApply <<
                    ". Flags: " <<
                    candidateIter->flags;
                if (account.retryPenalty &&
                        candidateIter->retriesRemaining > 2)
                    candidateIter->retriesRemaining = 1;
                else
                    --candidateIter->retriesRemaining;
                candidateIter->lastResult = txnResult;
                if (account.dropPenalty &&
                    account.transactions.size() > 1 && isFull<95>())
                {
                    
                    auto dropRIter = account.transactions.rbegin();
                    assert(dropRIter->second.account == candidateIter->account);
                    JLOG(j_.warn()) <<
                        "Queue is nearly full, and transaction " <<
                        candidateIter->txID << " failed with " <<
                        transToken(txnResult) <<
                        ". Removing last item of account " <<
                        account.account;
                    auto endIter = byFee_.iterator_to(dropRIter->second);
                    assert(endIter != candidateIter);
                    erase(endIter);

                }
                ++candidateIter;
            }

        }
        else
        {
            break;
        }
    }

    return ledgerChanged;
}

TxQ::Metrics
TxQ::getMetrics(OpenView const& view) const
{
    Metrics result;

    std::lock_guard<std::mutex> lock(mutex_);

    auto const snapshot = feeMetrics_.getSnapshot();

    result.txCount = byFee_.size();
    result.txQMaxSize = maxSize_;
    result.txInLedger = view.txCount();
    result.txPerLedger = snapshot.txnsExpected;
    result.referenceFeeLevel = baseLevel;
    result.minProcessingFeeLevel = isFull() ? byFee_.rbegin()->feeLevel + 1 :
        baseLevel;
    result.medFeeLevel = snapshot.escalationMultiplier;
    result.openLedgerFeeLevel = FeeMetrics::scaleFeeLevel(snapshot, view);

    return result;
}

auto
TxQ::getAccountTxs(AccountID const& account, ReadView const& view) const
    -> std::map<TxSeq, AccountTxDetails const>
{
    std::lock_guard<std::mutex> lock(mutex_);

    auto accountIter = byAccount_.find(account);
    if (accountIter == byAccount_.end() ||
        accountIter->second.transactions.empty())
        return {};

    std::map<TxSeq, AccountTxDetails const> result;

    for (auto const& tx : accountIter->second.transactions)
    {
        result.emplace(tx.first, [&]
        {
            AccountTxDetails resultTx;
            resultTx.feeLevel = tx.second.feeLevel;
            if (tx.second.lastValid)
                resultTx.lastValid.emplace(*tx.second.lastValid);
            if (tx.second.consequences)
                resultTx.consequences.emplace(*tx.second.consequences);
            return resultTx;
        }());
    }
    return result;
}

auto
TxQ::getTxs(ReadView const& view) const
-> std::vector<TxDetails>
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (byFee_.empty())
        return {};

    std::vector<TxDetails> result;
    result.reserve(byFee_.size());

    for (auto const& tx : byFee_)
    {
        result.emplace_back([&]
        {
            TxDetails resultTx;
            resultTx.feeLevel = tx.feeLevel;
            if (tx.lastValid)
                resultTx.lastValid.emplace(*tx.lastValid);
            if (tx.consequences)
                resultTx.consequences.emplace(*tx.consequences);
            resultTx.account = tx.account;
            resultTx.txn = tx.txn;
            resultTx.retriesRemaining = tx.retriesRemaining;
            BOOST_ASSERT(tx.pfresult);
            resultTx.preflightResult = tx.pfresult->ter;
            if (tx.lastResult)
                resultTx.lastResult.emplace(*tx.lastResult);
            return resultTx;
        }());
    }
    return result;
}


Json::Value
TxQ::doRPC(Application& app) const
{
    using std::to_string;

    auto const view = app.openLedger().current();
    if (!view)
    {
        BOOST_ASSERT(false);
        return {};
    }

    auto const metrics = getMetrics(*view);

    Json::Value ret(Json::objectValue);

    auto& levels = ret[jss::levels] = Json::objectValue;

    ret[jss::ledger_current_index] = view->info().seq;
    ret[jss::expected_ledger_size] = to_string(metrics.txPerLedger);
    ret[jss::current_ledger_size] = to_string(metrics.txInLedger);
    ret[jss::current_queue_size] = to_string(metrics.txCount);
    if (metrics.txQMaxSize)
        ret[jss::max_queue_size] = to_string(*metrics.txQMaxSize);

    levels[jss::reference_level] = to_string(metrics.referenceFeeLevel);
    levels[jss::minimum_level] = to_string(metrics.minProcessingFeeLevel);
    levels[jss::median_level] = to_string(metrics.medFeeLevel);
    levels[jss::open_ledger_level] = to_string(metrics.openLedgerFeeLevel);

    auto const baseFee = view->fees().base;
    auto& drops = ret[jss::drops] = Json::Value();

    drops[jss::base_fee] = to_string(mulDiv(
        metrics.referenceFeeLevel, baseFee,
            metrics.referenceFeeLevel).second);
    drops[jss::minimum_fee] = to_string(mulDiv(
        metrics.minProcessingFeeLevel, baseFee,
            metrics.referenceFeeLevel).second);
    drops[jss::median_fee] = to_string(mulDiv(
        metrics.medFeeLevel, baseFee,
            metrics.referenceFeeLevel).second);
    auto escalatedFee = mulDiv(
        metrics.openLedgerFeeLevel, baseFee,
            metrics.referenceFeeLevel).second;
    if (mulDiv(escalatedFee, metrics.referenceFeeLevel,
            baseFee).second < metrics.openLedgerFeeLevel)
        ++escalatedFee;

    drops[jss::open_ledger_fee] = to_string(escalatedFee);

    return ret;
}


TxQ::Setup
setup_TxQ(Config const& config)
{
    TxQ::Setup setup;
    auto const& section = config.section("transaction_queue");
    set(setup.ledgersInQueue, "ledgers_in_queue", section);
    set(setup.queueSizeMin, "minimum_queue_size", section);
    set(setup.retrySequencePercent, "retry_sequence_percent", section);
    set(setup.multiTxnPercent, "multi_txn_percent", section);
    set(setup.minimumEscalationMultiplier,
        "minimum_escalation_multiplier", section);
    set(setup.minimumTxnInLedger, "minimum_txn_in_ledger", section);
    set(setup.minimumTxnInLedgerSA,
        "minimum_txn_in_ledger_standalone", section);
    set(setup.targetTxnInLedger, "target_txn_in_ledger", section);
    std::uint32_t max;
    if (set(max, "maximum_txn_in_ledger", section))
    {
        if (max < setup.minimumTxnInLedger)
        {
            Throw<std::runtime_error>(
                "The minimum number of low-fee transactions allowed "
                "per ledger (minimum_txn_in_ledger) exceeds "
                "the maximum number of low-fee transactions allowed per "
                "ledger (maximum_txn_in_ledger)."
            );
        }
        if (max < setup.minimumTxnInLedgerSA)
        {
            Throw<std::runtime_error>(
                "The minimum number of low-fee transactions allowed "
                "per ledger (minimum_txn_in_ledger_standalone) exceeds "
                "the maximum number of low-fee transactions allowed per "
                "ledger (maximum_txn_in_ledger)."
                );
        }

        setup.maximumTxnInLedger.emplace(max);
    }

    
    set(setup.normalConsensusIncreasePercent,
        "normal_consensus_increase_percent", section);
    setup.normalConsensusIncreasePercent = boost::algorithm::clamp(
        setup.normalConsensusIncreasePercent, 0, 1000);

    
    set(setup.slowConsensusDecreasePercent, "slow_consensus_decrease_percent",
        section);
    setup.slowConsensusDecreasePercent = boost::algorithm::clamp(
        setup.slowConsensusDecreasePercent, 0, 100);

    set(setup.maximumTxnPerAccount, "maximum_txn_per_account", section);
    set(setup.minimumLastLedgerBuffer,
        "minimum_last_ledger_buffer", section);
    set(setup.zeroBaseFeeTransactionFeeLevel,
        "zero_basefee_transaction_feelevel", section);

    setup.standAlone = config.standalone();
    return setup;
}


std::unique_ptr<TxQ>
make_TxQ(TxQ::Setup const& setup, beast::Journal j)
{
    return std::make_unique<TxQ>(setup, std::move(j));
}

} 
























