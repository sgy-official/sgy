

#include <ripple/basics/Log.h>
#include <ripple/consensus/Consensus.h>

namespace ripple {

bool
shouldCloseLedger(
    bool anyTransactions,
    std::size_t prevProposers,
    std::size_t proposersClosed,
    std::size_t proposersValidated,
    std::chrono::milliseconds prevRoundTime,
    std::chrono::milliseconds
        timeSincePrevClose,              
    std::chrono::milliseconds openTime,  
    std::chrono::milliseconds idleInterval,
    ConsensusParms const& parms,
    beast::Journal j)
{
    using namespace std::chrono_literals;
    if ((prevRoundTime < -1s) || (prevRoundTime > 10min) ||
        (timeSincePrevClose > 10min))
    {
        JLOG(j.warn()) << "shouldCloseLedger Trans="
                       << (anyTransactions ? "yes" : "no")
                       << " Prop: " << prevProposers << "/" << proposersClosed
                       << " Secs: " << timeSincePrevClose.count()
                       << " (last: " << prevRoundTime.count() << ")";
        return true;
    }

    if ((proposersClosed + proposersValidated) > (prevProposers / 2))
    {
        JLOG(j.trace()) << "Others have closed";
        return true;
    }

    if (!anyTransactions)
    {
        return timeSincePrevClose >= idleInterval;  
    }

    if (openTime < parms.ledgerMIN_CLOSE)
    {
        JLOG(j.debug()) << "Must wait minimum time before closing";
        return false;
    }

    if (openTime < (prevRoundTime / 2))
    {
        JLOG(j.debug()) << "Ledger has not been open long enough";
        return false;
    }

    return true;
}

bool
checkConsensusReached(
    std::size_t agreeing,
    std::size_t total,
    bool count_self,
    std::size_t minConsensusPct)
{
    if (total == 0)
        return true;

    if (count_self)
    {
        ++agreeing;
        ++total;
    }

    std::size_t currentPercentage = (agreeing * 100) / total;

    return currentPercentage >= minConsensusPct;
}

ConsensusState
checkConsensus(
    std::size_t prevProposers,
    std::size_t currentProposers,
    std::size_t currentAgree,
    std::size_t currentFinished,
    std::chrono::milliseconds previousAgreeTime,
    std::chrono::milliseconds currentAgreeTime,
    ConsensusParms const& parms,
    bool proposing,
    beast::Journal j)
{
    JLOG(j.trace()) << "checkConsensus: prop=" << currentProposers << "/"
                    << prevProposers << " agree=" << currentAgree
                    << " validated=" << currentFinished
                    << " time=" << currentAgreeTime.count() << "/"
                    << previousAgreeTime.count();

    if (currentAgreeTime <= parms.ledgerMIN_CONSENSUS)
        return ConsensusState::No;

    if (currentProposers < (prevProposers * 3 / 4))
    {
        if (currentAgreeTime < (previousAgreeTime + parms.ledgerMIN_CONSENSUS))
        {
            JLOG(j.trace()) << "too fast, not enough proposers";
            return ConsensusState::No;
        }
    }

    if (checkConsensusReached(
            currentAgree, currentProposers, proposing, parms.minCONSENSUS_PCT))
    {
        JLOG(j.debug()) << "normal consensus";
        return ConsensusState::Yes;
    }

    if (checkConsensusReached(
            currentFinished, currentProposers, false, parms.minCONSENSUS_PCT))
    {
        JLOG(j.warn()) << "We see no consensus, but 80% of nodes have moved on";
        return ConsensusState::MovedOn;
    }

    JLOG(j.trace()) << "no consensus";
    return ConsensusState::No;
}

}  
























