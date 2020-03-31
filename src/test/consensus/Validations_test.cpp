

#include <ripple/basics/tagged_integer.h>
#include <ripple/beast/clock/manual_clock.h>
#include <ripple/beast/unit_test.h>
#include <ripple/consensus/Validations.h>
#include <test/csf/Validation.h>

#include <memory>
#include <tuple>
#include <type_traits>
#include <vector>

namespace ripple {
namespace test {
namespace csf {
class Validations_test : public beast::unit_test::suite
{
    using clock_type = beast::abstract_clock<std::chrono::steady_clock> const;

    static NetClock::time_point
    toNetClock(clock_type const& c)
    {
        using namespace std::chrono;
        return NetClock::time_point(duration_cast<NetClock::duration>(
            c.now().time_since_epoch() + 86400s));
    }

    class Node
    {
        clock_type const& c_;
        PeerID nodeID_;
        bool trusted_ = true;
        std::size_t signIdx_ = 1;
        boost::optional<std::uint32_t> loadFee_;

    public:
        Node(PeerID nodeID, clock_type const& c) : c_(c), nodeID_(nodeID)
        {
        }

        void
        untrust()
        {
            trusted_ = false;
        }

        void
        trust()
        {
            trusted_ = true;
        }

        void
        setLoadFee(std::uint32_t fee)
        {
            loadFee_ = fee;
        }

        PeerID
        nodeID() const
        {
            return nodeID_;
        }

        void
        advanceKey()
        {
            signIdx_++;
        }

        PeerKey
        currKey() const
        {
            return std::make_pair(nodeID_, signIdx_);
        }

        PeerKey
        masterKey() const
        {
            return std::make_pair(nodeID_, 0);
        }
        NetClock::time_point
        now() const
        {
            return toNetClock(c_);
        }

        Validation
        validate(
            Ledger::ID id,
            Ledger::Seq seq,
            NetClock::duration signOffset,
            NetClock::duration seenOffset,
            bool full) const
        {
            Validation v{id,
                         seq,
                         now() + signOffset,
                         now() + seenOffset,
                         currKey(),
                         nodeID_,
                         full,
                         loadFee_};
            if (trusted_)
                v.setTrusted();
            return v;
        }

        Validation
        validate(
            Ledger ledger,
            NetClock::duration signOffset,
            NetClock::duration seenOffset) const
        {
            return validate(
                ledger.id(), ledger.seq(), signOffset, seenOffset, true);
        }

        Validation
        validate(Ledger ledger) const
        {
            return validate(
                ledger.id(),
                ledger.seq(),
                NetClock::duration{0},
                NetClock::duration{0},
                true);
        }

        Validation
        partial(Ledger ledger) const
        {
            return validate(
                ledger.id(),
                ledger.seq(),
                NetClock::duration{0},
                NetClock::duration{0},
                false);
        }
    };

    struct StaleData
    {
        std::vector<Validation> stale;
        hash_map<PeerID, Validation> flushed;
    };

    class Adaptor
    {
        StaleData& staleData_;
        clock_type& c_;
        LedgerOracle& oracle_;

    public:
        struct Mutex
        {
            void
            lock()
            {
            }

            void
            unlock()
            {
            }
        };

        using Validation = csf::Validation;
        using Ledger = csf::Ledger;

        Adaptor(StaleData& sd, clock_type& c, LedgerOracle& o)
            : staleData_{sd}, c_{c}, oracle_{o}
        {
        }

        NetClock::time_point
        now() const
        {
            return toNetClock(c_);
        }

        void
        onStale(Validation&& v)
        {
            staleData_.stale.emplace_back(std::move(v));
        }

        void
        flush(hash_map<PeerID, Validation>&& remaining)
        {
            staleData_.flushed = std::move(remaining);
        }

        boost::optional<Ledger>
        acquire(Ledger::ID const& id)
        {
            return oracle_.lookup(id);
        }
    };

    using TestValidations = Validations<Adaptor>;

    class TestHarness
    {
        StaleData staleData_;
        ValidationParms p_;
        beast::manual_clock<std::chrono::steady_clock> clock_;
        TestValidations tv_;
        PeerID nextNodeId_{0};

    public:
        explicit TestHarness(LedgerOracle& o)
            : tv_(p_, clock_, staleData_, clock_, o)
        {
        }

        ValStatus
        add(Validation const& v)
        {
            return tv_.add(v.nodeID(), v);
        }

        TestValidations&
        vals()
        {
            return tv_;
        }

        Node
        makeNode()
        {
            return Node(nextNodeId_++, clock_);
        }

        ValidationParms
        parms() const
        {
            return p_;
        }

        auto&
        clock()
        {
            return clock_;
        }

        std::vector<Validation> const&
        stale() const
        {
            return staleData_.stale;
        }

        hash_map<PeerID, Validation> const&
        flushed() const
        {
            return staleData_.flushed;
        }
    };

    Ledger const genesisLedger{Ledger::MakeGenesis{}};

    void
    testAddValidation()
    {
        using namespace std::chrono_literals;

        testcase("Add validation");
        LedgerHistoryHelper h;
        Ledger ledgerA = h["a"];
        Ledger ledgerAB = h["ab"];
        Ledger ledgerAZ = h["az"];
        Ledger ledgerABC = h["abc"];
        Ledger ledgerABCD = h["abcd"];
        Ledger ledgerABCDE = h["abcde"];

        {
            TestHarness harness(h.oracle);
            Node n = harness.makeNode();

            auto const v = n.validate(ledgerA);

            BEAST_EXPECT(ValStatus::current == harness.add(v));

            BEAST_EXPECT(ValStatus::badSeq == harness.add(v));

            harness.clock().advance(1s);
            BEAST_EXPECT(harness.stale().empty());

            BEAST_EXPECT(
                ValStatus::current == harness.add(n.validate(ledgerAB)));

            BEAST_EXPECT(harness.stale().size() == 1);

            BEAST_EXPECT(harness.stale()[0].ledgerID() == ledgerA.id());


            BEAST_EXPECT(
                harness.vals().numTrustedForLedger(ledgerAB.id()) == 1);
            BEAST_EXPECT(
                harness.vals().numTrustedForLedger(ledgerABC.id()) == 0);

            n.advanceKey();

            harness.clock().advance(1s);

            BEAST_EXPECT(
                ValStatus::badSeq == harness.add(n.validate(ledgerAB)));
            BEAST_EXPECT(ValStatus::badSeq == harness.add(n.partial(ledgerAB)));

            harness.clock().advance(1s);
            BEAST_EXPECT(
                ValStatus::current == harness.add(n.validate(ledgerABC)));
            BEAST_EXPECT(
                harness.vals().numTrustedForLedger(ledgerAB.id()) == 1);
            BEAST_EXPECT(
                harness.vals().numTrustedForLedger(ledgerABC.id()) == 1);

            harness.clock().advance(2s);
            auto const valABCDE = n.validate(ledgerABCDE);

            harness.clock().advance(4s);
            auto const valABCD = n.validate(ledgerABCD);

            BEAST_EXPECT(ValStatus::current == harness.add(valABCD));

            BEAST_EXPECT(ValStatus::stale == harness.add(valABCDE));
        }

        {

            TestHarness harness(h.oracle);
            Node n = harness.makeNode();

            BEAST_EXPECT(
                ValStatus::current == harness.add(n.validate(ledgerA)));

            BEAST_EXPECT(
                ValStatus::stale ==
                harness.add(n.validate(ledgerAB, -1s, -1s)));

            BEAST_EXPECT(
                ValStatus::current ==
                harness.add(n.validate(ledgerABC, 1s, 1s)));
        }

        {
            TestHarness harness(h.oracle);
            Node n = harness.makeNode();

            BEAST_EXPECT(
                ValStatus::stale ==
                harness.add(n.validate(
                    ledgerA, -harness.parms().validationCURRENT_EARLY, 0s)));

            BEAST_EXPECT(
                ValStatus::stale ==
                harness.add(n.validate(
                    ledgerA, harness.parms().validationCURRENT_WALL, 0s)));

            BEAST_EXPECT(
                ValStatus::stale ==
                harness.add(n.validate(
                    ledgerA, 0s, harness.parms().validationCURRENT_LOCAL)));
        }

        {
            for (bool doFull : {true, false})
            {
                TestHarness harness(h.oracle);
                Node n = harness.makeNode();

                auto process = [&](Ledger& lgr) {
                    if (doFull)
                        return harness.add(n.validate(lgr));
                    return harness.add(n.partial(lgr));
                };

                BEAST_EXPECT(ValStatus::current == process(ledgerABC));
                harness.clock().advance(1s);
                BEAST_EXPECT(ledgerAB.seq() < ledgerABC.seq());
                BEAST_EXPECT(ValStatus::badSeq == process(ledgerAB));

                BEAST_EXPECT(ValStatus::badSeq == process(ledgerAZ));
                harness.clock().advance(
                    harness.parms().validationSET_EXPIRES + 1ms);
                BEAST_EXPECT(ValStatus::current == process(ledgerAZ));
            }
        }
    }

    void
    testOnStale()
    {
        testcase("Stale validation");

        LedgerHistoryHelper h;
        Ledger ledgerA = h["a"];
        Ledger ledgerAB = h["ab"];

        using Trigger = std::function<void(TestValidations&)>;

        std::vector<Trigger> triggers = {
            [&](TestValidations& vals) { vals.currentTrusted(); },
            [&](TestValidations& vals) { vals.getCurrentNodeIDs(); },
            [&](TestValidations& vals) { vals.getPreferred(genesisLedger); },
            [&](TestValidations& vals) {
                vals.getNodesAfter(ledgerA, ledgerA.id());
            }};
        for (Trigger trigger : triggers)
        {
            TestHarness harness(h.oracle);
            Node n = harness.makeNode();

            BEAST_EXPECT(
                ValStatus::current == harness.add(n.validate(ledgerAB)));
            trigger(harness.vals());
            BEAST_EXPECT(
                harness.vals().getNodesAfter(ledgerA, ledgerA.id()) == 1);
            BEAST_EXPECT(
                harness.vals().getPreferred(genesisLedger) ==
                std::make_pair(ledgerAB.seq(), ledgerAB.id()));
            BEAST_EXPECT(harness.stale().empty());
            harness.clock().advance(harness.parms().validationCURRENT_LOCAL);

            trigger(harness.vals());

            BEAST_EXPECT(harness.stale().size() == 1);
            BEAST_EXPECT(harness.stale()[0].ledgerID() == ledgerAB.id());
            BEAST_EXPECT(
                harness.vals().getNodesAfter(ledgerA, ledgerA.id()) == 0);
            BEAST_EXPECT(
                harness.vals().getPreferred(genesisLedger) == boost::none);
        }
    }

    void
    testGetNodesAfter()
    {

        using namespace std::chrono_literals;
        testcase("Get nodes after");

        LedgerHistoryHelper h;
        Ledger ledgerA = h["a"];
        Ledger ledgerAB = h["ab"];
        Ledger ledgerABC = h["abc"];
        Ledger ledgerAD = h["ad"];

        TestHarness harness(h.oracle);
        Node a = harness.makeNode(), b = harness.makeNode(),
             c = harness.makeNode(), d = harness.makeNode();
        c.untrust();

        BEAST_EXPECT(ValStatus::current == harness.add(a.validate(ledgerA)));
        BEAST_EXPECT(ValStatus::current == harness.add(b.validate(ledgerA)));
        BEAST_EXPECT(ValStatus::current == harness.add(c.validate(ledgerA)));
        BEAST_EXPECT(ValStatus::current == harness.add(d.partial(ledgerA)));

        for (Ledger const& ledger : {ledgerA, ledgerAB, ledgerABC, ledgerAD})
            BEAST_EXPECT(
                harness.vals().getNodesAfter(ledger, ledger.id()) == 0);

        harness.clock().advance(5s);

        BEAST_EXPECT(ValStatus::current == harness.add(a.validate(ledgerAB)));
        BEAST_EXPECT(ValStatus::current == harness.add(b.validate(ledgerABC)));
        BEAST_EXPECT(ValStatus::current == harness.add(c.validate(ledgerAB)));
        BEAST_EXPECT(ValStatus::current == harness.add(d.partial(ledgerABC)));

        BEAST_EXPECT(harness.vals().getNodesAfter(ledgerA, ledgerA.id()) == 3);
        BEAST_EXPECT(
            harness.vals().getNodesAfter(ledgerAB, ledgerAB.id()) == 2);
        BEAST_EXPECT(
            harness.vals().getNodesAfter(ledgerABC, ledgerABC.id()) == 0);
        BEAST_EXPECT(
            harness.vals().getNodesAfter(ledgerAD, ledgerAD.id()) == 0);

        BEAST_EXPECT(harness.vals().getNodesAfter(ledgerAD, ledgerA.id()) == 1);
        BEAST_EXPECT(
            harness.vals().getNodesAfter(ledgerAD, ledgerAB.id()) == 2);
    }

    void
    testCurrentTrusted()
    {
        using namespace std::chrono_literals;
        testcase("Current trusted validations");

        LedgerHistoryHelper h;
        Ledger ledgerA = h["a"];
        Ledger ledgerB = h["b"];
        Ledger ledgerAC = h["ac"];

        TestHarness harness(h.oracle);
        Node a = harness.makeNode(), b = harness.makeNode();
        b.untrust();

        BEAST_EXPECT(ValStatus::current == harness.add(a.validate(ledgerA)));
        BEAST_EXPECT(ValStatus::current == harness.add(b.validate(ledgerB)));

        BEAST_EXPECT(harness.vals().currentTrusted().size() == 1);
        BEAST_EXPECT(
            harness.vals().currentTrusted()[0].ledgerID() == ledgerA.id());
        BEAST_EXPECT(harness.vals().currentTrusted()[0].seq() == ledgerA.seq());

        harness.clock().advance(3s);

        for (auto const& node : {a, b})
            BEAST_EXPECT(
                ValStatus::current == harness.add(node.validate(ledgerAC)));

        BEAST_EXPECT(harness.vals().currentTrusted().size() == 1);
        BEAST_EXPECT(
            harness.vals().currentTrusted()[0].ledgerID() == ledgerAC.id());
        BEAST_EXPECT(
            harness.vals().currentTrusted()[0].seq() == ledgerAC.seq());

        harness.clock().advance(harness.parms().validationCURRENT_LOCAL);
        BEAST_EXPECT(harness.vals().currentTrusted().empty());
    }

    void
    testGetCurrentPublicKeys()
    {
        using namespace std::chrono_literals;
        testcase("Current public keys");

        LedgerHistoryHelper h;
        Ledger ledgerA = h["a"];
        Ledger ledgerAC = h["ac"];

        TestHarness harness(h.oracle);
        Node a = harness.makeNode(), b = harness.makeNode();
        b.untrust();

        for (auto const& node : {a, b})
            BEAST_EXPECT(
                ValStatus::current == harness.add(node.validate(ledgerA)));

        {
            hash_set<PeerID> const expectedKeys = {a.nodeID(), b.nodeID()};
            BEAST_EXPECT(harness.vals().getCurrentNodeIDs() == expectedKeys);
        }

        harness.clock().advance(3s);

        a.advanceKey();
        b.advanceKey();

        for (auto const& node : {a, b})
            BEAST_EXPECT(
                ValStatus::current == harness.add(node.partial(ledgerAC)));

        {
            hash_set<PeerID> const expectedKeys = {a.nodeID(), b.nodeID()};
            BEAST_EXPECT(harness.vals().getCurrentNodeIDs() == expectedKeys);
        }

        harness.clock().advance(harness.parms().validationCURRENT_LOCAL);
        BEAST_EXPECT(harness.vals().getCurrentNodeIDs().empty());
    }

    void
    testTrustedByLedgerFunctions()
    {
        using namespace std::chrono_literals;
        testcase("By ledger functions");


        LedgerHistoryHelper h;
        TestHarness harness(h.oracle);

        Node a = harness.makeNode(), b = harness.makeNode(),
             c = harness.makeNode(), d = harness.makeNode(),
             e = harness.makeNode();

        c.untrust();
        a.setLoadFee(12);
        b.setLoadFee(1);
        c.setLoadFee(12);
        e.setLoadFee(12);

        hash_map<Ledger::ID, std::vector<Validation>> trustedValidations;

        auto sorted = [](auto vec) {
            std::sort(vec.begin(), vec.end());
            return vec;
        };
        auto compare = [&]() {
            for (auto& it : trustedValidations)
            {
                auto const& id = it.first;
                auto const& expectedValidations = it.second;

                BEAST_EXPECT(
                    harness.vals().numTrustedForLedger(id) ==
                    expectedValidations.size());
                BEAST_EXPECT(
                    sorted(harness.vals().getTrustedForLedger(id)) ==
                    sorted(expectedValidations));

                std::uint32_t baseFee = 0;
                std::vector<uint32_t> expectedFees;
                for (auto const& val : expectedValidations)
                {
                    expectedFees.push_back(val.loadFee().value_or(baseFee));
                }

                BEAST_EXPECT(
                    sorted(harness.vals().fees(id, baseFee)) ==
                    sorted(expectedFees));
            }
        };

        Ledger ledgerA = h["a"];
        Ledger ledgerB = h["b"];
        Ledger ledgerAC = h["ac"];

        trustedValidations[Ledger::ID{100}] = {};

        for (auto const& node : {a, b, c})
        {
            auto const val = node.validate(ledgerA);
            BEAST_EXPECT(ValStatus::current == harness.add(val));
            if (val.trusted())
                trustedValidations[val.ledgerID()].emplace_back(val);
        }
        {
            auto const val = d.validate(ledgerB);
            BEAST_EXPECT(ValStatus::current == harness.add(val));
            trustedValidations[val.ledgerID()].emplace_back(val);
        }
        {
            BEAST_EXPECT(ValStatus::current == harness.add(e.partial(ledgerA)));
        }

        harness.clock().advance(5s);
        for (auto const& node : {a, b, c})
        {
            auto const val = node.validate(ledgerAC);
            BEAST_EXPECT(ValStatus::current == harness.add(val));
            if (val.trusted())
                trustedValidations[val.ledgerID()].emplace_back(val);
        }
        {
            BEAST_EXPECT(ValStatus::badSeq == harness.add(d.partial(ledgerA)));
        }
        {
            BEAST_EXPECT(
                ValStatus::current == harness.add(e.partial(ledgerAC)));
        }

        compare();
    }

    void
    testExpire()
    {
        testcase("Expire validations");
        LedgerHistoryHelper h;
        TestHarness harness(h.oracle);
        Node a = harness.makeNode();

        Ledger ledgerA = h["a"];

        BEAST_EXPECT(ValStatus::current == harness.add(a.validate(ledgerA)));
        BEAST_EXPECT(harness.vals().numTrustedForLedger(ledgerA.id()));
        harness.clock().advance(harness.parms().validationSET_EXPIRES);
        harness.vals().expire();
        BEAST_EXPECT(!harness.vals().numTrustedForLedger(ledgerA.id()));
    }

    void
    testFlush()
    {
        using namespace std::chrono_literals;
        testcase("Flush validations");

        LedgerHistoryHelper h;
        TestHarness harness(h.oracle);
        Node a = harness.makeNode(), b = harness.makeNode(),
             c = harness.makeNode();
        c.untrust();

        Ledger ledgerA = h["a"];
        Ledger ledgerAB = h["ab"];

        hash_map<PeerID, Validation> expected;
        for (auto const& node : {a, b, c})
        {
            auto const val = node.validate(ledgerA);
            BEAST_EXPECT(ValStatus::current == harness.add(val));
            expected.emplace(node.nodeID(), val);
        }
        Validation staleA = expected.find(a.nodeID())->second;

        harness.clock().advance(1s);
        auto newVal = a.validate(ledgerAB);
        BEAST_EXPECT(ValStatus::current == harness.add(newVal));
        expected.find(a.nodeID())->second = newVal;

        harness.vals().flush();

        BEAST_EXPECT(harness.stale().size() == 1);
        BEAST_EXPECT(harness.stale()[0] == staleA);
        BEAST_EXPECT(harness.stale()[0].nodeID() == a.nodeID());

        auto const& flushed = harness.flushed();

        BEAST_EXPECT(flushed == expected);
    }

    void
    testGetPreferredLedger()
    {
        using namespace std::chrono_literals;
        testcase("Preferred Ledger");

        LedgerHistoryHelper h;
        TestHarness harness(h.oracle);
        Node a = harness.makeNode(), b = harness.makeNode(),
             c = harness.makeNode(), d = harness.makeNode();
        c.untrust();

        Ledger ledgerA = h["a"];
        Ledger ledgerB = h["b"];
        Ledger ledgerAC = h["ac"];
        Ledger ledgerACD = h["acd"];

        using Seq = Ledger::Seq;
        using ID = Ledger::ID;

        auto pref = [](Ledger ledger) {
            return std::make_pair(ledger.seq(), ledger.id());
        };

        BEAST_EXPECT(harness.vals().getPreferred(ledgerA) == boost::none);

        BEAST_EXPECT(ValStatus::current == harness.add(a.validate(ledgerB)));
        BEAST_EXPECT(harness.vals().getPreferred(ledgerA) == pref(ledgerB));
        BEAST_EXPECT(harness.vals().getPreferred(ledgerB) == pref(ledgerB));

        BEAST_EXPECT(
            harness.vals().getPreferred(ledgerA, Seq{10}) == ledgerA.id());

        BEAST_EXPECT(ValStatus::current == harness.add(b.validate(ledgerA)));
        BEAST_EXPECT(ValStatus::current == harness.add(c.validate(ledgerA)));
        BEAST_EXPECT(ledgerB.id() > ledgerA.id());
        BEAST_EXPECT(harness.vals().getPreferred(ledgerA) == pref(ledgerB));
        BEAST_EXPECT(harness.vals().getPreferred(ledgerB) == pref(ledgerB));

        BEAST_EXPECT(ValStatus::current == harness.add(d.partial(ledgerA)));
        BEAST_EXPECT(harness.vals().getPreferred(ledgerA) == pref(ledgerA));
        BEAST_EXPECT(harness.vals().getPreferred(ledgerB) == pref(ledgerA));

        harness.clock().advance(5s);

        for (auto const& node : {a, b, c, d})
            BEAST_EXPECT(
                ValStatus::current == harness.add(node.validate(ledgerAC)));
        BEAST_EXPECT(harness.vals().getPreferred(ledgerA) == pref(ledgerA));
        BEAST_EXPECT(harness.vals().getPreferred(ledgerB) == pref(ledgerAC));
        BEAST_EXPECT(harness.vals().getPreferred(ledgerACD) == pref(ledgerACD));

        harness.clock().advance(5s);
        for (auto const& node : {a, b, c, d})
            BEAST_EXPECT(
                ValStatus::current == harness.add(node.validate(ledgerACD)));
        for (auto const& ledger : {ledgerA, ledgerB, ledgerACD})
            BEAST_EXPECT(
                harness.vals().getPreferred(ledger) == pref(ledgerACD));
    }

    void
    testGetPreferredLCL()
    {
        using namespace std::chrono_literals;
        testcase("Get preferred LCL");

        LedgerHistoryHelper h;
        TestHarness harness(h.oracle);
        Node a = harness.makeNode();

        Ledger ledgerA = h["a"];
        Ledger ledgerB = h["b"];
        Ledger ledgerC = h["c"];

        using ID = Ledger::ID;
        using Seq = Ledger::Seq;

        hash_map<ID, std::uint32_t> peerCounts;

        BEAST_EXPECT(
            harness.vals().getPreferredLCL(ledgerA, Seq{0}, peerCounts) ==
            ledgerA.id());

        ++peerCounts[ledgerB.id()];

        BEAST_EXPECT(
            harness.vals().getPreferredLCL(ledgerA, Seq{0}, peerCounts) ==
            ledgerB.id());

        ++peerCounts[ledgerC.id()];
        BEAST_EXPECT(ledgerC.id() > ledgerB.id());

        BEAST_EXPECT(
            harness.vals().getPreferredLCL(ledgerA, Seq{0}, peerCounts) ==
            ledgerC.id());

        peerCounts[ledgerC.id()] += 1000;

        BEAST_EXPECT(ValStatus::current == harness.add(a.validate(ledgerA)));
        BEAST_EXPECT(
            harness.vals().getPreferredLCL(ledgerA, Seq{0}, peerCounts) ==
            ledgerA.id());
        BEAST_EXPECT(
            harness.vals().getPreferredLCL(ledgerB, Seq{0}, peerCounts) ==
            ledgerA.id());
        BEAST_EXPECT(
            harness.vals().getPreferredLCL(ledgerC, Seq{0}, peerCounts) ==
            ledgerA.id());

        BEAST_EXPECT(
            harness.vals().getPreferredLCL(ledgerB, Seq{2}, peerCounts) ==
            ledgerB.id());
    }

    void
    testAcquireValidatedLedger()
    {
        using namespace std::chrono_literals;
        testcase("Acquire validated ledger");

        LedgerHistoryHelper h;
        TestHarness harness(h.oracle);
        Node a = harness.makeNode();
        Node b = harness.makeNode();

        using ID = Ledger::ID;
        using Seq = Ledger::Seq;

        Validation val = a.validate(ID{2}, Seq{2}, 0s, 0s, true);

        BEAST_EXPECT(ValStatus::current == harness.add(val));
        BEAST_EXPECT(harness.vals().numTrustedForLedger(ID{2}) == 1);
        BEAST_EXPECT(harness.vals().getNodesAfter(genesisLedger, ID{0}) == 0);
        BEAST_EXPECT(
            harness.vals().getPreferred(genesisLedger) ==
            std::make_pair(Seq{2}, ID{2}));

        BEAST_EXPECT(
            ValStatus::current ==
            harness.add(b.validate(ID{3}, Seq{2}, 0s, 0s, true)));
        BEAST_EXPECT(
            harness.vals().getPreferred(genesisLedger) ==
            std::make_pair(Seq{2}, ID{3}));

        Ledger ledgerAB = h["ab"];
        BEAST_EXPECT(harness.vals().getNodesAfter(genesisLedger, ID{0}) == 1);

        harness.clock().advance(5s);
        Validation val2 = a.validate(ID{4}, Seq{4}, 0s, 0s, true);
        BEAST_EXPECT(ValStatus::current == harness.add(val2));
        BEAST_EXPECT(harness.vals().numTrustedForLedger(ID{4}) == 1);
        BEAST_EXPECT(
            harness.vals().getPreferred(genesisLedger) ==
            std::make_pair(ledgerAB.seq(), ledgerAB.id()));

        Validation val3 = b.validate(ID{4}, Seq{4}, 0s, 0s, true);
        BEAST_EXPECT(ValStatus::current == harness.add(val3));
        BEAST_EXPECT(harness.vals().numTrustedForLedger(ID{4}) == 2);
        BEAST_EXPECT(
            harness.vals().getPreferred(genesisLedger) ==
            std::make_pair(ledgerAB.seq(), ledgerAB.id()));

        harness.clock().advance(5s);
        Ledger ledgerABCDE = h["abcde"];
        BEAST_EXPECT(ValStatus::current == harness.add(a.partial(ledgerABCDE)));
        BEAST_EXPECT(ValStatus::current == harness.add(b.partial(ledgerABCDE)));
        BEAST_EXPECT(
            harness.vals().getPreferred(genesisLedger) ==
            std::make_pair(ledgerABCDE.seq(), ledgerABCDE.id()));
    }

    void
    testNumTrustedForLedger()
    {
        testcase("NumTrustedForLedger");
        LedgerHistoryHelper h;
        TestHarness harness(h.oracle);
        Node a = harness.makeNode();
        Node b = harness.makeNode();
        Ledger ledgerA = h["a"];

        BEAST_EXPECT(ValStatus::current == harness.add(a.partial(ledgerA)));
        BEAST_EXPECT(harness.vals().numTrustedForLedger(ledgerA.id()) == 0);

        BEAST_EXPECT(ValStatus::current == harness.add(b.validate(ledgerA)));
        BEAST_EXPECT(harness.vals().numTrustedForLedger(ledgerA.id()) == 1);
    }

    void
    testSeqEnforcer()
    {
        testcase("SeqEnforcer");
        using Seq = Ledger::Seq;
        using namespace std::chrono;

        beast::manual_clock<steady_clock> clock;
        SeqEnforcer<Seq> enforcer;

        ValidationParms p;

        BEAST_EXPECT(enforcer(clock.now(), Seq{1}, p));
        BEAST_EXPECT(enforcer(clock.now(), Seq{10}, p));
        BEAST_EXPECT(!enforcer(clock.now(), Seq{5}, p));
        BEAST_EXPECT(!enforcer(clock.now(), Seq{9}, p));
        clock.advance(p.validationSET_EXPIRES - 1ms);
        BEAST_EXPECT(!enforcer(clock.now(), Seq{1}, p));
        clock.advance(2ms);
        BEAST_EXPECT(enforcer(clock.now(), Seq{1}, p));
    }

    void
    testTrustChanged()
    {
        testcase("TrustChanged");
        using namespace std::chrono;

        auto checker = [this](
                           TestValidations& vals,
                           hash_set<PeerID> const& listed,
                           std::vector<Validation> const& trustedVals) {
            Ledger::ID testID = trustedVals.empty() ? this->genesisLedger.id()
                                                    : trustedVals[0].ledgerID();
            BEAST_EXPECT(vals.currentTrusted() == trustedVals);
            BEAST_EXPECT(vals.getCurrentNodeIDs() == listed);
            BEAST_EXPECT(
                vals.getNodesAfter(this->genesisLedger, genesisLedger.id()) ==
                trustedVals.size());
            if (trustedVals.empty())
                BEAST_EXPECT(
                    vals.getPreferred(this->genesisLedger) == boost::none);
            else
                BEAST_EXPECT(
                    vals.getPreferred(this->genesisLedger)->second == testID);
            BEAST_EXPECT(vals.getTrustedForLedger(testID) == trustedVals);
            BEAST_EXPECT(
                vals.numTrustedForLedger(testID) == trustedVals.size());
        };

        {
            LedgerHistoryHelper h;
            TestHarness harness(h.oracle);
            Node a = harness.makeNode();
            Ledger ledgerAB = h["ab"];
            Validation v = a.validate(ledgerAB);
            BEAST_EXPECT(ValStatus::current == harness.add(v));

            hash_set<PeerID> listed({a.nodeID()});
            std::vector<Validation> trustedVals({v});
            checker(harness.vals(), listed, trustedVals);

            trustedVals.clear();
            harness.vals().trustChanged({}, {a.nodeID()});
            checker(harness.vals(), listed, trustedVals);
        }

        {
            LedgerHistoryHelper h;
            TestHarness harness(h.oracle);
            Node a = harness.makeNode();
            a.untrust();
            Ledger ledgerAB = h["ab"];
            Validation v = a.validate(ledgerAB);
            BEAST_EXPECT(ValStatus::current == harness.add(v));

            hash_set<PeerID> listed({a.nodeID()});
            std::vector<Validation> trustedVals;
            checker(harness.vals(), listed, trustedVals);

            trustedVals.push_back(v);
            harness.vals().trustChanged({a.nodeID()}, {});
            checker(harness.vals(), listed, trustedVals);
        }

        {
            LedgerHistoryHelper h;
            TestHarness harness(h.oracle);
            Node a = harness.makeNode();
            Validation v =
                a.validate(Ledger::ID{2}, Ledger::Seq{2}, 0s, 0s, true);
            BEAST_EXPECT(ValStatus::current == harness.add(v));

            hash_set<PeerID> listed({a.nodeID()});
            std::vector<Validation> trustedVals({v});
            auto& vals = harness.vals();
            BEAST_EXPECT(vals.currentTrusted() == trustedVals);
            BEAST_EXPECT(
                vals.getPreferred(genesisLedger)->second == v.ledgerID());
            BEAST_EXPECT(
                vals.getNodesAfter(genesisLedger, genesisLedger.id()) == 0);

            trustedVals.clear();
            harness.vals().trustChanged({}, {a.nodeID()});
            h["ab"];
            BEAST_EXPECT(vals.currentTrusted() == trustedVals);
            BEAST_EXPECT(vals.getPreferred(genesisLedger) == boost::none);
            BEAST_EXPECT(
                vals.getNodesAfter(genesisLedger, genesisLedger.id()) == 0);
        }
    }

    void
    run() override
    {
        testAddValidation();
        testOnStale();
        testGetNodesAfter();
        testCurrentTrusted();
        testGetCurrentPublicKeys();
        testTrustedByLedgerFunctions();
        testExpire();
        testFlush();
        testGetPreferredLedger();
        testGetPreferredLCL();
        testAcquireValidatedLedger();
        testNumTrustedForLedger();
        testSeqEnforcer();
        testTrustChanged();
    }
};

BEAST_DEFINE_TESTSUITE(Validations, consensus, ripple);
}  
}  
}  
























