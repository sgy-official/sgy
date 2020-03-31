
#ifndef RIPPLE_TEST_CSF_COLLECTORS_H_INCLUDED
#define RIPPLE_TEST_CSF_COLLECTORS_H_INCLUDED

#include <ripple/basics/UnorderedContainers.h>
#include <boost/optional.hpp>
#include <chrono>
#include <ostream>
#include <test/csf/Histogram.h>
#include <test/csf/SimTime.h>
#include <test/csf/events.h>
#include <tuple>

namespace ripple {
namespace test {
namespace csf {




template <class... Cs>
class Collectors
{
    std::tuple<Cs&...> cs;

    template <class C, class E>
    static void
    apply(C& c, PeerID who, SimTime when, E e)
    {
        c.on(who, when, e);
    }

    template <std::size_t... Is, class E>
    static void
    apply(
        std::tuple<Cs&...>& cs,
        PeerID who,
        SimTime when,
        E e,
        std::index_sequence<Is...>)
    {
        (void)std::array<int, sizeof...(Cs)>{
            {((apply(std::get<Is>(cs), who, when, e)), 0)...}};
    }

public:
    
    Collectors(Cs&... cs) : cs(std::tie(cs...))
    {
    }

    template <class E>
    void
    on(PeerID who, SimTime when, E e)
    {
        apply(cs, who, when, e, std::index_sequence_for<Cs...>{});
    }
};


template <class... Cs>
Collectors<Cs...>
makeCollectors(Cs&... cs)
{
    return Collectors<Cs...>(cs...);
}


template <class CollectorType>
struct CollectByNode
{
    std::map<PeerID, CollectorType> byNode;

    CollectorType&
    operator[](PeerID who)
    {
        return byNode[who];
    }

    CollectorType const&
    operator[](PeerID who) const
    {
        return byNode[who];
    }
    template <class E>
    void
    on(PeerID who, SimTime when, E const& e)
    {
        byNode[who].on(who, when, e);
    }

};


struct NullCollector
{
    template <class E>
    void
    on(PeerID, SimTime, E const& e)
    {
    }
};


struct SimDurationCollector
{
    bool init = false;
    SimTime start;
    SimTime stop;

    template <class E>
    void
    on(PeerID, SimTime when, E const& e)
    {
        if (!init)
        {
            start = when;
            init = true;
        }
        else
            stop = when;
    }
};


struct TxCollector
{
    std::size_t submitted{0};
    std::size_t accepted{0};
    std::size_t validated{0};

    struct Tracker
    {
        Tx tx;
        SimTime submitted;
        boost::optional<SimTime> accepted;
        boost::optional<SimTime> validated;

        Tracker(Tx tx_, SimTime submitted_) : tx{tx_}, submitted{submitted_}
        {
        }
    };

    hash_map<Tx::ID, Tracker> txs;

    using Hist = Histogram<SimTime::duration>;
    Hist submitToAccept;
    Hist submitToValidate;

    template <class E>
    void
    on(PeerID, SimTime when, E const& e)
    {
    }

    void
    on(PeerID who, SimTime when, SubmitTx const& e)
    {

        if (txs.emplace(e.tx.id(), Tracker{e.tx, when}).second)
        {
            submitted++;
        }
    }

    void
    on(PeerID who, SimTime when, AcceptLedger const& e)
    {
        for (auto const& tx : e.ledger.txs())
        {
            auto it = txs.find(tx.id());
            if (it != txs.end() && !it->second.accepted)
            {
                Tracker& tracker = it->second;
                tracker.accepted = when;
                accepted++;

                submitToAccept.insert(*tracker.accepted - tracker.submitted);
            }
        }
    }

    void
    on(PeerID who, SimTime when, FullyValidateLedger const& e)
    {
        for (auto const& tx : e.ledger.txs())
        {
            auto it = txs.find(tx.id());
            if (it != txs.end() && !it->second.validated)
            {
                Tracker& tracker = it->second;
                assert(tracker.accepted);

                tracker.validated = when;
                validated++;
                submitToValidate.insert(*tracker.validated - tracker.submitted);
            }
        }
    }

    std::size_t
    orphaned() const
    {
        return std::count_if(txs.begin(), txs.end(), [](auto const& it) {
            return !it.second.accepted;
        });
    }

    std::size_t
    unvalidated() const
    {
        return std::count_if(txs.begin(), txs.end(), [](auto const& it) {
            return !it.second.validated;
        });
    }

    template <class T>
    void
    report(SimDuration simDuration, T& log, bool printBreakline = false)
    {
        using namespace std::chrono;
        auto perSec = [&simDuration](std::size_t count)
        {
            return double(count)/duration_cast<seconds>(simDuration).count();
        };

        auto fmtS = [](SimDuration dur)
        {
            return duration_cast<duration<float>>(dur).count();
        };

        if (printBreakline)
        {
            log << std::setw(11) << std::setfill('-') << "-" <<  "-"
                << std::setw(7) << std::setfill('-') << "-" <<  "-"
                << std::setw(7) << std::setfill('-') << "-" <<  "-"
                << std::setw(36) << std::setfill('-') << "-"
                << std::endl;
            log << std::setfill(' ');
        }

        log << std::left
            << std::setw(11) << "TxStats" <<  "|"
            << std::setw(7) << "Count" <<  "|"
            << std::setw(7) << "Per Sec" <<  "|"
            << std::setw(15) << "Latency (sec)"
            << std::right
            << std::setw(7) << "10-ile"
            << std::setw(7) << "50-ile"
            << std::setw(7) << "90-ile"
            << std::left
            << std::endl;

        log << std::setw(11) << std::setfill('-') << "-" <<  "|"
            << std::setw(7) << std::setfill('-') << "-" <<  "|"
            << std::setw(7) << std::setfill('-') << "-" <<  "|"
            << std::setw(36) << std::setfill('-') << "-"
            << std::endl;
        log << std::setfill(' ');

        log << std::left <<
            std::setw(11) << "Submit " << "|"
            << std::right
            << std::setw(7) << submitted << "|"
            << std::setw(7) << std::setprecision(2) << perSec(submitted) << "|"
            << std::setw(36) << "" << std::endl;

        log << std::left
            << std::setw(11) << "Accept " << "|"
            << std::right
            << std::setw(7) << accepted << "|"
            << std::setw(7) << std::setprecision(2) << perSec(accepted) << "|"
            << std::setw(15) << std::left << "From Submit" << std::right
            << std::setw(7) << std::setprecision(2) << fmtS(submitToAccept.percentile(0.1f))
            << std::setw(7) << std::setprecision(2) << fmtS(submitToAccept.percentile(0.5f))
            << std::setw(7) << std::setprecision(2) << fmtS(submitToAccept.percentile(0.9f))
            << std::endl;

        log << std::left
            << std::setw(11) << "Validate " << "|"
            << std::right
            << std::setw(7) << validated << "|"
            << std::setw(7) << std::setprecision(2) << perSec(validated) << "|"
            << std::setw(15) << std::left << "From Submit" << std::right
            << std::setw(7) << std::setprecision(2) << fmtS(submitToValidate.percentile(0.1f))
            << std::setw(7) << std::setprecision(2) << fmtS(submitToValidate.percentile(0.5f))
            << std::setw(7) << std::setprecision(2) << fmtS(submitToValidate.percentile(0.9f))
            << std::endl;

        log << std::left
            << std::setw(11) << "Orphan" << "|"
            << std::right
            << std::setw(7) << orphaned() << "|"
            << std::setw(7) << "" << "|"
            << std::setw(36) << std::endl;

        log << std::left
            << std::setw(11) << "Unvalidated" << "|"
            << std::right
            << std::setw(7) << unvalidated() << "|"
            << std::setw(7) << "" << "|"
            << std::setw(43) << std::endl;

        log << std::setw(11) << std::setfill('-') << "-" <<  "-"
            << std::setw(7) << std::setfill('-') << "-" <<  "-"
            << std::setw(7) << std::setfill('-') << "-" <<  "-"
            << std::setw(36) << std::setfill('-') << "-"
            << std::endl;
        log << std::setfill(' ');
    }

    template <class T, class Tag>
    void
    csv(SimDuration simDuration, T& log, Tag const& tag, bool printHeaders = false)
    {
        using namespace std::chrono;
        auto perSec = [&simDuration](std::size_t count)
        {
            return double(count)/duration_cast<seconds>(simDuration).count();
        };

        auto fmtS = [](SimDuration dur)
        {
            return duration_cast<duration<float>>(dur).count();
        };

        if(printHeaders)
        {
            log << "tag" << ","
                << "txNumSubmitted" << ","
                << "txNumAccepted" << ","
                << "txNumValidated" << ","
                << "txNumOrphaned" << ","
                << "txUnvalidated" << ","
                << "txRateSumbitted" << ","
                << "txRateAccepted" << ","
                << "txRateValidated" << ","
                << "txLatencySubmitToAccept10Pctl" << ","
                << "txLatencySubmitToAccept50Pctl" << ","
                << "txLatencySubmitToAccept90Pctl" << ","
                << "txLatencySubmitToValidatet10Pctl" << ","
                << "txLatencySubmitToValidatet50Pctl" << ","
                << "txLatencySubmitToValidatet90Pctl"
                << std::endl;
        }


        log << tag << ","
            << submitted << ","
            << accepted << ","
            << validated << ","
            << orphaned() << ","
            << unvalidated() << ","
            << std::setprecision(2) << perSec(submitted) << ","
            << std::setprecision(2) << perSec(accepted) << ","
            << std::setprecision(2) << perSec(validated) << ","
            << std::setprecision(2) << fmtS(submitToAccept.percentile(0.1f)) << ","
            << std::setprecision(2) << fmtS(submitToAccept.percentile(0.5f)) << ","
            << std::setprecision(2) << fmtS(submitToAccept.percentile(0.9f)) << ","
            << std::setprecision(2) << fmtS(submitToValidate.percentile(0.1f)) << ","
            << std::setprecision(2) << fmtS(submitToValidate.percentile(0.5f)) << ","
            << std::setprecision(2) << fmtS(submitToValidate.percentile(0.9f)) << ","
            << std::endl;
    }
};


struct LedgerCollector
{
    std::size_t accepted{0};
    std::size_t fullyValidated{0};

    struct Tracker
    {
        SimTime accepted;
        boost::optional<SimTime> fullyValidated;

        Tracker(SimTime accepted_) : accepted{accepted_}
        {
        }
    };

    hash_map<Ledger::ID, Tracker> ledgers_;

    using Hist = Histogram<SimTime::duration>;
    Hist acceptToFullyValid;
    Hist acceptToAccept;
    Hist fullyValidToFullyValid;

    template <class E>
    void
    on(PeerID, SimTime, E const& e)
    {
    }

    void
    on(PeerID who, SimTime when, AcceptLedger const& e)
    {
        if (ledgers_.emplace(e.ledger.id(), Tracker{when}).second)
        {
            ++accepted;
            if (e.prior.id() == e.ledger.parentID())
            {
                auto const it = ledgers_.find(e.ledger.parentID());
                if (it != ledgers_.end())
                {
                    acceptToAccept.insert(when - it->second.accepted);
                }
            }
        }
    }

    void
    on(PeerID who, SimTime when, FullyValidateLedger const& e)
    {
        if (e.prior.id() == e.ledger.parentID())
        {
            auto const it = ledgers_.find(e.ledger.id());
            assert(it != ledgers_.end());
            auto& tracker = it->second;
            if (!tracker.fullyValidated)
            {
                ++fullyValidated;
                tracker.fullyValidated = when;
                acceptToFullyValid.insert(when - tracker.accepted);

                auto const parentIt = ledgers_.find(e.ledger.parentID());
                if (parentIt != ledgers_.end())
                {
                    auto& parentTracker = parentIt->second;
                    if (parentTracker.fullyValidated)
                    {
                        fullyValidToFullyValid.insert(
                            when - *parentTracker.fullyValidated);
                    }
                }
            }
        }
    }

    std::size_t
    unvalidated() const
    {
        return std::count_if(
            ledgers_.begin(), ledgers_.end(), [](auto const& it) {
                return !it.second.fullyValidated;
            });
    }

    template <class T>
    void
    report(SimDuration simDuration, T& log, bool printBreakline = false)
    {
        using namespace std::chrono;
        auto perSec = [&simDuration](std::size_t count)
        {
            return double(count)/duration_cast<seconds>(simDuration).count();
        };

        auto fmtS = [](SimDuration dur)
        {
            return duration_cast<duration<float>>(dur).count();
        };

        if (printBreakline)
        {
            log << std::setw(11) << std::setfill('-') << "-" <<  "-"
                << std::setw(7) << std::setfill('-') << "-" <<  "-"
                << std::setw(7) << std::setfill('-') << "-" <<  "-"
                << std::setw(36) << std::setfill('-') << "-"
                << std::endl;
            log << std::setfill(' ');
        }

        log << std::left
            << std::setw(11) << "LedgerStats" <<  "|"
            << std::setw(7)  << "Count" <<  "|"
            << std::setw(7)  << "Per Sec" <<  "|"
            << std::setw(15) << "Latency (sec)"
            << std::right
            << std::setw(7) << "10-ile"
            << std::setw(7) << "50-ile"
            << std::setw(7) << "90-ile"
            << std::left
            << std::endl;

        log << std::setw(11) << std::setfill('-') << "-" <<  "|"
            << std::setw(7) << std::setfill('-') << "-" <<  "|"
            << std::setw(7) << std::setfill('-') << "-" <<  "|"
            << std::setw(36) << std::setfill('-') << "-"
            << std::endl;
        log << std::setfill(' ');

         log << std::left
            << std::setw(11) << "Accept " << "|"
            << std::right
            << std::setw(7) << accepted << "|"
            << std::setw(7) << std::setprecision(2) << perSec(accepted) << "|"
            << std::setw(15) << std::left << "From Accept" << std::right
            << std::setw(7) << std::setprecision(2) << fmtS(acceptToAccept.percentile(0.1f))
            << std::setw(7) << std::setprecision(2) << fmtS(acceptToAccept.percentile(0.5f))
            << std::setw(7) << std::setprecision(2) << fmtS(acceptToAccept.percentile(0.9f))
            << std::endl;

        log << std::left
            << std::setw(11) << "Validate " << "|"
            << std::right
            << std::setw(7) << fullyValidated << "|"
            << std::setw(7) << std::setprecision(2) << perSec(fullyValidated) << "|"
            << std::setw(15) << std::left << "From Validate " << std::right
            << std::setw(7) << std::setprecision(2) << fmtS(fullyValidToFullyValid.percentile(0.1f))
            << std::setw(7) << std::setprecision(2) << fmtS(fullyValidToFullyValid.percentile(0.5f))
            << std::setw(7) << std::setprecision(2) << fmtS(fullyValidToFullyValid.percentile(0.9f))
            << std::endl;

        log << std::setw(11) << std::setfill('-') << "-" <<  "-"
            << std::setw(7) << std::setfill('-') << "-" <<  "-"
            << std::setw(7) << std::setfill('-') << "-" <<  "-"
            << std::setw(36) << std::setfill('-') << "-"
            << std::endl;
        log << std::setfill(' ');
    }

    template <class T, class Tag>
    void
    csv(SimDuration simDuration, T& log, Tag const& tag, bool printHeaders = false)
    {
        using namespace std::chrono;
        auto perSec = [&simDuration](std::size_t count)
        {
            return double(count)/duration_cast<seconds>(simDuration).count();
        };

        auto fmtS = [](SimDuration dur)
        {
            return duration_cast<duration<float>>(dur).count();
        };

        if(printHeaders)
        {
            log << "tag" << ","
                << "ledgerNumAccepted" << ","
                << "ledgerNumFullyValidated" << ","
                << "ledgerRateAccepted" << ","
                << "ledgerRateFullyValidated" << ","
                << "ledgerLatencyAcceptToAccept10Pctl" << ","
                << "ledgerLatencyAcceptToAccept50Pctl" << ","
                << "ledgerLatencyAcceptToAccept90Pctl" << ","
                << "ledgerLatencyFullyValidToFullyValid10Pctl" << ","
                << "ledgerLatencyFullyValidToFullyValid50Pctl" << ","
                << "ledgerLatencyFullyValidToFullyValid90Pctl"
                << std::endl;
        }

        log << tag << ","
            << accepted << ","
            << fullyValidated << ","
            << std::setprecision(2) << perSec(accepted) << ","
            << std::setprecision(2) << perSec(fullyValidated) << ","
            << std::setprecision(2) << fmtS(acceptToAccept.percentile(0.1f)) << ","
            << std::setprecision(2) << fmtS(acceptToAccept.percentile(0.5f)) << ","
            << std::setprecision(2) << fmtS(acceptToAccept.percentile(0.9f)) << ","
            << std::setprecision(2) << fmtS(fullyValidToFullyValid.percentile(0.1f)) << ","
            << std::setprecision(2) << fmtS(fullyValidToFullyValid.percentile(0.5f)) << ","
            << std::setprecision(2) << fmtS(fullyValidToFullyValid.percentile(0.9f))
            << std::endl;
    }
};


struct StreamCollector
{
    std::ostream& out;

    template <class E>
    void
    on(PeerID, SimTime, E const& e)
    {
    }

    void
    on(PeerID who, SimTime when, AcceptLedger const& e)
    {
        out << when.time_since_epoch().count() << ": Node " << who << " accepted "
            << "L" << e.ledger.id() << " " << e.ledger.txs() << "\n";
    }

    void
    on(PeerID who, SimTime when, FullyValidateLedger const& e)
    {
        out << when.time_since_epoch().count() << ": Node " << who
            << " fully-validated " << "L"<< e.ledger.id() << " " << e.ledger.txs()
            << "\n";
    }
};


struct JumpCollector
{
    struct Jump
    {
        PeerID id;
        SimTime when;
        Ledger from;
        Ledger to;
    };

    std::vector<Jump> closeJumps;
    std::vector<Jump> fullyValidatedJumps;

    template <class E>
    void
    on(PeerID, SimTime, E const& e)
    {
    }

    void
    on(PeerID who, SimTime when, AcceptLedger const& e)
    {
        if(e.ledger.parentID() != e.prior.id())
            closeJumps.emplace_back(Jump{who, when, e.prior, e.ledger});
    }

    void
    on(PeerID who, SimTime when, FullyValidateLedger const& e)
    {
        if (e.ledger.parentID() != e.prior.id())
            fullyValidatedJumps.emplace_back(
                Jump{who, when, e.prior, e.ledger});
    }
};

}  
}  
}  

#endif








