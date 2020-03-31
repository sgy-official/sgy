

#include <ripple/app/ledger/BuildLedger.h>
#include <ripple/app/ledger/Ledger.h>
#include <ripple/app/ledger/LedgerReplay.h>
#include <ripple/app/ledger/OpenLedger.h>
#include <ripple/app/main/Application.h>
#include <ripple/app/misc/CanonicalTXSet.h>
#include <ripple/app/tx/apply.h>
#include <ripple/protocol/Feature.h>

namespace ripple {


template <class ApplyTxs>
std::shared_ptr<Ledger>
buildLedgerImpl(
    std::shared_ptr<Ledger const> const& parent,
    NetClock::time_point closeTime,
    const bool closeTimeCorrect,
    NetClock::duration closeResolution,
    Application& app,
    beast::Journal j,
    ApplyTxs&& applyTxs)
{
    auto built = std::make_shared<Ledger>(*parent, closeTime);

    if (built->rules().enabled(featureSHAMapV2) && !built->stateMap().is_v2())
        built->make_v2();


    {
        OpenView accum(&*built);
        assert(!accum.open());
        applyTxs(accum, built);
        accum.apply(*built);
    }

    built->updateSkipList();
    {

        int const asf = built->stateMap().flushDirty(
            hotACCOUNT_NODE, built->info().seq);
        int const tmf = built->txMap().flushDirty(
            hotTRANSACTION_NODE, built->info().seq);
        JLOG(j.debug()) << "Flushed " << asf << " accounts and " << tmf
                        << " transaction nodes";
    }
    built->unshare();

    built->setAccepted(
        closeTime, closeResolution, closeTimeCorrect, app.config());

    return built;
}



std::size_t
applyTransactions(
    Application& app,
    std::shared_ptr<Ledger const> const& built,
    CanonicalTXSet& txns,
    std::set<TxID>& failed,
    OpenView& view,
    beast::Journal j)
{
    bool certainRetry = true;
    std::size_t count = 0;

    for (int pass = 0; pass < LEDGER_TOTAL_PASSES; ++pass)
    {
        JLOG(j.debug())
            << (certainRetry ? "Pass: " : "Final pass: ") << pass
            << " begins (" << txns.size() << " transactions)";
        int changes = 0;

        auto it = txns.begin();

        while (it != txns.end())
        {
            auto const txid = it->first.getTXID();

            try
            {
                if (pass == 0 && built->txExists(txid))
                {
                    it = txns.erase(it);
                    continue;
                }

                switch (applyTransaction(
                    app, view, *it->second, certainRetry, tapNONE, j))
                {
                    case ApplyResult::Success:
                        it = txns.erase(it);
                        ++changes;
                        break;

                    case ApplyResult::Fail:
                        failed.insert(txid);
                        it = txns.erase(it);
                        break;

                    case ApplyResult::Retry:
                        ++it;
                }
            }
            catch (std::exception const&)
            {
                JLOG(j.warn()) << "Transaction " << txid << " throws";
                failed.insert(txid);
                it = txns.erase(it);
            }
        }

        JLOG(j.debug())
            << (certainRetry ? "Pass: " : "Final pass: ") << pass
            << " completed (" << changes << " changes)";

        if (!changes && !certainRetry)
            break;

        if (!changes || (pass >= LEDGER_RETRY_PASSES))
            certainRetry = false;
    }

    assert(txns.empty() || !certainRetry);
    return count;
}

std::shared_ptr<Ledger>
buildLedger(
    std::shared_ptr<Ledger const> const& parent,
    NetClock::time_point closeTime,
    const bool closeTimeCorrect,
    NetClock::duration closeResolution,
    Application& app,
    CanonicalTXSet& txns,
    std::set<TxID>& failedTxns,
    beast::Journal j)
{
    JLOG(j.debug()) << "Report: Transaction Set = " << txns.key()
                    << ", close " << closeTime.time_since_epoch().count()
                    << (closeTimeCorrect ? "" : " (incorrect)");

    return buildLedgerImpl(parent, closeTime, closeTimeCorrect,
        closeResolution, app, j,
        [&](OpenView& accum, std::shared_ptr<Ledger> const& built)
        {
            JLOG(j.debug())
                << "Attempting to apply " << txns.size()
                << " transactions";

            auto const applied = applyTransactions(app, built, txns,
                failedTxns, accum, j);

            if (txns.size() || txns.size())
                JLOG(j.debug())
                    << "Applied " << applied << " transactions; "
                    << failedTxns.size() << " failed and "
                    << txns.size() << " will be retried.";
            else
                JLOG(j.debug())
                    << "Applied " << applied
                    << " transactions.";
        });
}

std::shared_ptr<Ledger>
buildLedger(
    LedgerReplay const& replayData,
    ApplyFlags applyFlags,
    Application& app,
    beast::Journal j)
{
    auto const& replayLedger = replayData.replay();

    JLOG(j.debug()) << "Report: Replay Ledger " << replayLedger->info().hash;

    return buildLedgerImpl(
        replayData.parent(),
        replayLedger->info().closeTime,
        ((replayLedger->info().closeFlags & sLCF_NoConsensusTime) == 0),
        replayLedger->info().closeTimeResolution,
        app,
        j,
        [&](OpenView& accum, std::shared_ptr<Ledger> const& built)
        {
            for (auto& tx : replayData.orderedTxns())
                applyTransaction(app, accum, *tx.second, false, applyFlags, j);
        });
}

}  
























