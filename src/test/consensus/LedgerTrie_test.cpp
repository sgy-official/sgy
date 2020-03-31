
#include <ripple/beast/unit_test.h>
#include <ripple/consensus/LedgerTrie.h>
#include <random>
#include <test/csf/ledgers.h>
#include <unordered_map>

namespace ripple {
namespace test {

class LedgerTrie_test : public beast::unit_test::suite
{
    void
    testInsert()
    {
        using namespace csf;
        {
            LedgerTrie<Ledger> t;
            LedgerHistoryHelper h;
            t.insert(h["abc"]);
            BEAST_EXPECT(t.checkInvariants());
            BEAST_EXPECT(t.tipSupport(h["abc"]) == 1);
            BEAST_EXPECT(t.branchSupport(h["abc"]) == 1);

            t.insert(h["abc"]);
            BEAST_EXPECT(t.checkInvariants());
            BEAST_EXPECT(t.tipSupport(h["abc"]) == 2);
            BEAST_EXPECT(t.branchSupport(h["abc"]) == 2);
        }
        {
            LedgerTrie<Ledger> t;
            LedgerHistoryHelper h;
            t.insert(h["abc"]);
            BEAST_EXPECT(t.checkInvariants());
            t.insert(h["abcd"]);
            BEAST_EXPECT(t.checkInvariants());

            BEAST_EXPECT(t.tipSupport(h["abc"]) == 1);
            BEAST_EXPECT(t.branchSupport(h["abc"]) == 2);
            BEAST_EXPECT(t.tipSupport(h["abcd"]) == 1);
            BEAST_EXPECT(t.branchSupport(h["abcd"]) == 1);

            t.insert(h["abce"]);
            BEAST_EXPECT(t.tipSupport(h["abc"]) == 1);
            BEAST_EXPECT(t.branchSupport(h["abc"]) == 3);
            BEAST_EXPECT(t.tipSupport(h["abcd"]) == 1);
            BEAST_EXPECT(t.branchSupport(h["abcd"]) == 1);
            BEAST_EXPECT(t.tipSupport(h["abce"]) == 1);
            BEAST_EXPECT(t.branchSupport(h["abce"]) == 1);
        }
        {
            LedgerTrie<Ledger> t;
            LedgerHistoryHelper h;
            t.insert(h["abcd"]);
            BEAST_EXPECT(t.checkInvariants());
            t.insert(h["abcdf"]);
            BEAST_EXPECT(t.checkInvariants());

            BEAST_EXPECT(t.tipSupport(h["abcd"]) == 1);
            BEAST_EXPECT(t.branchSupport(h["abcd"]) == 2);
            BEAST_EXPECT(t.tipSupport(h["abcdf"]) == 1);
            BEAST_EXPECT(t.branchSupport(h["abcdf"]) == 1);

            t.insert(h["abc"]);
            BEAST_EXPECT(t.checkInvariants());

            BEAST_EXPECT(t.tipSupport(h["abc"]) == 1);
            BEAST_EXPECT(t.branchSupport(h["abc"]) == 3);
            BEAST_EXPECT(t.tipSupport(h["abcd"]) == 1);
            BEAST_EXPECT(t.branchSupport(h["abcd"]) == 2);
            BEAST_EXPECT(t.tipSupport(h["abcdf"]) == 1);
            BEAST_EXPECT(t.branchSupport(h["abcdf"]) == 1);
        }
        {
            LedgerTrie<Ledger> t;
            LedgerHistoryHelper h;
            t.insert(h["abcd"]);
            BEAST_EXPECT(t.checkInvariants());
            t.insert(h["abce"]);
            BEAST_EXPECT(t.checkInvariants());

            BEAST_EXPECT(t.tipSupport(h["abc"]) == 0);
            BEAST_EXPECT(t.branchSupport(h["abc"]) == 2);
            BEAST_EXPECT(t.tipSupport(h["abcd"]) == 1);
            BEAST_EXPECT(t.branchSupport(h["abcd"]) == 1);
            BEAST_EXPECT(t.tipSupport(h["abce"]) == 1);
            BEAST_EXPECT(t.branchSupport(h["abce"]) == 1);
        }
        {

            LedgerTrie<Ledger> t;
            LedgerHistoryHelper h;
            t.insert(h["abcd"]);
            BEAST_EXPECT(t.checkInvariants());
            t.insert(h["abcde"]);
            BEAST_EXPECT(t.checkInvariants());
            t.insert(h["abcf"]);
            BEAST_EXPECT(t.checkInvariants());

            BEAST_EXPECT(t.tipSupport(h["abc"]) == 0);
            BEAST_EXPECT(t.branchSupport(h["abc"]) == 3);
            BEAST_EXPECT(t.tipSupport(h["abcd"]) == 1);
            BEAST_EXPECT(t.branchSupport(h["abcd"]) == 2);
            BEAST_EXPECT(t.tipSupport(h["abcf"]) == 1);
            BEAST_EXPECT(t.branchSupport(h["abcf"]) == 1);
            BEAST_EXPECT(t.tipSupport(h["abcde"]) == 1);
            BEAST_EXPECT(t.branchSupport(h["abcde"]) == 1);
        }

        {
            LedgerTrie<Ledger> t;
            LedgerHistoryHelper h;
            t.insert(h["ab"], 4);
            BEAST_EXPECT(t.tipSupport(h["ab"]) == 4);
            BEAST_EXPECT(t.branchSupport(h["ab"]) == 4);
            BEAST_EXPECT(t.tipSupport(h["a"]) == 0);
            BEAST_EXPECT(t.branchSupport(h["a"]) == 4);

            t.insert(h["abc"], 2);
            BEAST_EXPECT(t.tipSupport(h["abc"]) == 2);
            BEAST_EXPECT(t.branchSupport(h["abc"]) == 2);
            BEAST_EXPECT(t.tipSupport(h["ab"]) == 4);
            BEAST_EXPECT(t.branchSupport(h["ab"]) == 6);
            BEAST_EXPECT(t.tipSupport(h["a"]) == 0);
            BEAST_EXPECT(t.branchSupport(h["a"]) == 6);
        }
    }

    void
    testRemove()
    {
        using namespace csf;
        {
            LedgerTrie<Ledger> t;
            LedgerHistoryHelper h;
            t.insert(h["abc"]);

            BEAST_EXPECT(!t.remove(h["ab"]));
            BEAST_EXPECT(t.checkInvariants());
            BEAST_EXPECT(!t.remove(h["a"]));
            BEAST_EXPECT(t.checkInvariants());
        }
        {
            LedgerTrie<Ledger> t;
            LedgerHistoryHelper h;
            t.insert(h["abcd"]);
            t.insert(h["abce"]);

            BEAST_EXPECT(t.tipSupport(h["abc"]) == 0);
            BEAST_EXPECT(t.branchSupport(h["abc"]) == 2);
            BEAST_EXPECT(!t.remove(h["abc"]));
            BEAST_EXPECT(t.checkInvariants());
            BEAST_EXPECT(t.tipSupport(h["abc"]) == 0);
            BEAST_EXPECT(t.branchSupport(h["abc"]) == 2);
        }
        {
            LedgerTrie<Ledger> t;
            LedgerHistoryHelper h;
            t.insert(h["abc"], 2);

            BEAST_EXPECT(t.tipSupport(h["abc"]) == 2);
            BEAST_EXPECT(t.remove(h["abc"]));
            BEAST_EXPECT(t.checkInvariants());
            BEAST_EXPECT(t.tipSupport(h["abc"]) == 1);

            t.insert(h["abc"], 1);
            BEAST_EXPECT(t.tipSupport(h["abc"]) == 2);
            BEAST_EXPECT(t.remove(h["abc"], 2));
            BEAST_EXPECT(t.checkInvariants());
            BEAST_EXPECT(t.tipSupport(h["abc"]) == 0);

            t.insert(h["abc"], 3);
            BEAST_EXPECT(t.tipSupport(h["abc"]) == 3);
            BEAST_EXPECT(t.remove(h["abc"], 300));
            BEAST_EXPECT(t.checkInvariants());
            BEAST_EXPECT(t.tipSupport(h["abc"]) == 0);
        }
        {
            LedgerTrie<Ledger> t;
            LedgerHistoryHelper h;
            t.insert(h["ab"]);
            t.insert(h["abc"]);

            BEAST_EXPECT(t.tipSupport(h["ab"]) == 1);
            BEAST_EXPECT(t.branchSupport(h["ab"]) == 2);
            BEAST_EXPECT(t.tipSupport(h["abc"]) == 1);
            BEAST_EXPECT(t.branchSupport(h["abc"]) == 1);

            BEAST_EXPECT(t.remove(h["abc"]));
            BEAST_EXPECT(t.checkInvariants());
            BEAST_EXPECT(t.tipSupport(h["ab"]) == 1);
            BEAST_EXPECT(t.branchSupport(h["ab"]) == 1);
            BEAST_EXPECT(t.tipSupport(h["abc"]) == 0);
            BEAST_EXPECT(t.branchSupport(h["abc"]) == 0);
        }
        {
            LedgerTrie<Ledger> t;
            LedgerHistoryHelper h;
            t.insert(h["ab"]);
            t.insert(h["abc"]);
            t.insert(h["abcd"]);

            BEAST_EXPECT(t.tipSupport(h["abc"]) == 1);
            BEAST_EXPECT(t.branchSupport(h["abc"]) == 2);
            BEAST_EXPECT(t.tipSupport(h["abcd"]) == 1);
            BEAST_EXPECT(t.branchSupport(h["abcd"]) == 1);

            BEAST_EXPECT(t.remove(h["abc"]));
            BEAST_EXPECT(t.checkInvariants());
            BEAST_EXPECT(t.tipSupport(h["abc"]) == 0);
            BEAST_EXPECT(t.branchSupport(h["abc"]) == 1);
            BEAST_EXPECT(t.tipSupport(h["abcd"]) == 1);
            BEAST_EXPECT(t.branchSupport(h["abcd"]) == 1);
        }
        {
            LedgerTrie<Ledger> t;
            LedgerHistoryHelper h;
            t.insert(h["ab"]);
            t.insert(h["abc"]);
            t.insert(h["abcd"]);
            t.insert(h["abce"]);

            BEAST_EXPECT(t.tipSupport(h["abc"]) == 1);
            BEAST_EXPECT(t.branchSupport(h["abc"]) == 3);

            BEAST_EXPECT(t.remove(h["abc"]));
            BEAST_EXPECT(t.checkInvariants());
            BEAST_EXPECT(t.tipSupport(h["abc"]) == 0);
            BEAST_EXPECT(t.branchSupport(h["abc"]) == 2);
        }

        {
            LedgerTrie<Ledger> t;
            LedgerHistoryHelper h;
            t.insert(h["ab"]);
            t.insert(h["abc"]);
            t.insert(h["abd"]);
            BEAST_EXPECT(t.checkInvariants());
            t.remove(h["ab"]);
            BEAST_EXPECT(t.checkInvariants());
            BEAST_EXPECT(t.tipSupport(h["abc"]) == 1);
            BEAST_EXPECT(t.tipSupport(h["abd"]) == 1);
            BEAST_EXPECT(t.tipSupport(h["ab"]) == 0);
            BEAST_EXPECT(t.branchSupport(h["ab"]) == 2);

            t.remove(h["abd"]);
            BEAST_EXPECT(t.checkInvariants());

            BEAST_EXPECT(t.tipSupport(h["abc"]) == 1);
            BEAST_EXPECT(t.branchSupport(h["ab"]) == 1);
        }
    }

    void
    testEmpty()
    {
        using namespace csf;
        LedgerTrie<Ledger> t;
        LedgerHistoryHelper h;
        BEAST_EXPECT(t.empty());

        Ledger genesis = h[""];
        t.insert(genesis);
        BEAST_EXPECT(!t.empty());
        t.remove(genesis);
        BEAST_EXPECT(t.empty());

        t.insert(h["abc"]);
        BEAST_EXPECT(!t.empty());
        t.remove(h["abc"]);
        BEAST_EXPECT(t.empty());
    }

    void
    testSupport()
    {
        using namespace csf;
        using Seq = Ledger::Seq;

        LedgerTrie<Ledger> t;
        LedgerHistoryHelper h;
        BEAST_EXPECT(t.tipSupport(h["a"]) == 0);
        BEAST_EXPECT(t.tipSupport(h["axy"]) == 0);

        BEAST_EXPECT(t.branchSupport(h["a"]) == 0);
        BEAST_EXPECT(t.branchSupport(h["axy"]) == 0);

        t.insert(h["abc"]);
        BEAST_EXPECT(t.tipSupport(h["a"]) == 0);
        BEAST_EXPECT(t.tipSupport(h["ab"]) == 0);
        BEAST_EXPECT(t.tipSupport(h["abc"]) == 1);
        BEAST_EXPECT(t.tipSupport(h["abcd"]) == 0);

        BEAST_EXPECT(t.branchSupport(h["a"]) == 1);
        BEAST_EXPECT(t.branchSupport(h["ab"]) == 1);
        BEAST_EXPECT(t.branchSupport(h["abc"]) == 1);
        BEAST_EXPECT(t.branchSupport(h["abcd"]) == 0);

        t.insert(h["abe"]);
        BEAST_EXPECT(t.tipSupport(h["a"]) == 0);
        BEAST_EXPECT(t.tipSupport(h["ab"]) == 0);
        BEAST_EXPECT(t.tipSupport(h["abc"]) == 1);
        BEAST_EXPECT(t.tipSupport(h["abe"]) == 1);

        BEAST_EXPECT(t.branchSupport(h["a"]) == 2);
        BEAST_EXPECT(t.branchSupport(h["ab"]) == 2);
        BEAST_EXPECT(t.branchSupport(h["abc"]) == 1);
        BEAST_EXPECT(t.branchSupport(h["abe"]) == 1);

        t.remove(h["abc"]);
        BEAST_EXPECT(t.tipSupport(h["a"]) == 0);
        BEAST_EXPECT(t.tipSupport(h["ab"]) == 0);
        BEAST_EXPECT(t.tipSupport(h["abc"]) == 0);
        BEAST_EXPECT(t.tipSupport(h["abe"]) == 1);

        BEAST_EXPECT(t.branchSupport(h["a"]) == 1);
        BEAST_EXPECT(t.branchSupport(h["ab"]) == 1);
        BEAST_EXPECT(t.branchSupport(h["abc"]) == 0);
        BEAST_EXPECT(t.branchSupport(h["abe"]) == 1);
    }

    void
    testGetPreferred()
    {
        using namespace csf;
        using Seq = Ledger::Seq;
        {
            LedgerTrie<Ledger> t;
            BEAST_EXPECT(t.getPreferred(Seq{0}) == boost::none);
            BEAST_EXPECT(t.getPreferred(Seq{2}) == boost::none);
        }
        {
            LedgerTrie<Ledger> t;
            LedgerHistoryHelper h;
            Ledger genesis = h[""];
            t.insert(genesis);
            BEAST_EXPECT(t.getPreferred(Seq{0})->id == genesis.id());
            BEAST_EXPECT(t.remove(genesis));
            BEAST_EXPECT(t.getPreferred(Seq{0}) == boost::none);
            BEAST_EXPECT(!t.remove(genesis));
        }
        {
            LedgerTrie<Ledger> t;
            LedgerHistoryHelper h;
            t.insert(h["abc"]);
            BEAST_EXPECT(t.getPreferred(Seq{3})->id == h["abc"].id());
        }
        {
            LedgerTrie<Ledger> t;
            LedgerHistoryHelper h;
            t.insert(h["abc"]);
            t.insert(h["abcd"]);
            BEAST_EXPECT(t.getPreferred(Seq{3})->id == h["abc"].id());
            BEAST_EXPECT(t.getPreferred(Seq{4})->id == h["abc"].id());
        }
        {
            LedgerTrie<Ledger> t;
            LedgerHistoryHelper h;
            t.insert(h["abc"]);
            t.insert(h["abcd"], 2);
            BEAST_EXPECT(t.getPreferred(Seq{3})->id == h["abcd"].id());
            BEAST_EXPECT(t.getPreferred(Seq{4})->id == h["abcd"].id());
        }
        {
            LedgerTrie<Ledger> t;
            LedgerHistoryHelper h;
            t.insert(h["abc"]);
            t.insert(h["abcd"]);
            t.insert(h["abce"]);
            BEAST_EXPECT(t.getPreferred(Seq{3})->id == h["abc"].id());
            BEAST_EXPECT(t.getPreferred(Seq{4})->id == h["abc"].id());

            t.insert(h["abc"]);
            BEAST_EXPECT(t.getPreferred(Seq{3})->id == h["abc"].id());
            BEAST_EXPECT(t.getPreferred(Seq{4})->id == h["abc"].id());
        }
        {
            LedgerTrie<Ledger> t;
            LedgerHistoryHelper h;
            t.insert(h["abc"]);
            t.insert(h["abcd"], 2);
            t.insert(h["abce"]);
            BEAST_EXPECT(t.getPreferred(Seq{3})->id == h["abc"].id());
            BEAST_EXPECT(t.getPreferred(Seq{4})->id == h["abc"].id());

            t.insert(h["abcd"]);
            BEAST_EXPECT(t.getPreferred(Seq{3})->id == h["abcd"].id());
            BEAST_EXPECT(t.getPreferred(Seq{4})->id == h["abcd"].id());
        }
        {
            LedgerTrie<Ledger> t;
            LedgerHistoryHelper h;
            t.insert(h["abcd"], 2);
            t.insert(h["abce"], 2);

            BEAST_EXPECT(h["abce"].id() > h["abcd"].id());
            BEAST_EXPECT(t.getPreferred(Seq{4})->id == h["abce"].id());

            t.insert(h["abcd"]);
            BEAST_EXPECT(h["abce"].id() > h["abcd"].id());
            BEAST_EXPECT(t.getPreferred(Seq{4})->id == h["abcd"].id());
        }

        {
            LedgerTrie<Ledger> t;
            LedgerHistoryHelper h;
            t.insert(h["abc"]);
            t.insert(h["abcd"]);
            t.insert(h["abce"], 2);
            BEAST_EXPECT(h["abce"].id() > h["abcd"].id());
            BEAST_EXPECT(t.getPreferred(Seq{3})->id == h["abce"].id());
            BEAST_EXPECT(t.getPreferred(Seq{4})->id == h["abce"].id());

            t.remove(h["abce"]);
            t.insert(h["abcd"]);
            BEAST_EXPECT(t.getPreferred(Seq{3})->id == h["abc"].id());
            BEAST_EXPECT(t.getPreferred(Seq{4})->id == h["abc"].id());
        }

        {
            LedgerTrie<Ledger> t;
            LedgerHistoryHelper h;
            t.insert(h["abc"]);
            t.insert(h["abcd"], 2);
            t.insert(h["abcde"], 4);
            BEAST_EXPECT(t.getPreferred(Seq{3})->id == h["abcde"].id());
            BEAST_EXPECT(t.getPreferred(Seq{4})->id == h["abcde"].id());
            BEAST_EXPECT(t.getPreferred(Seq{5})->id == h["abcde"].id());
        }

        {
            LedgerTrie<Ledger> t;
            LedgerHistoryHelper h;
            t.insert(h["abc"]);
            t.insert(h["abcde"], 2);
            t.insert(h["abcfg"], 2);
            BEAST_EXPECT(t.getPreferred(Seq{3})->id == h["abc"].id());
            BEAST_EXPECT(t.getPreferred(Seq{4})->id == h["abc"].id());
            BEAST_EXPECT(t.getPreferred(Seq{5})->id == h["abc"].id());

            t.remove(h["abc"]);
            t.insert(h["abcd"]);

            BEAST_EXPECT(t.getPreferred(Seq{3})->id == h["abcde"].id());
            BEAST_EXPECT(t.getPreferred(Seq{4})->id == h["abcde"].id());

            BEAST_EXPECT(t.getPreferred(Seq{5})->id == h["abc"].id());
        }

        {
            
            LedgerTrie<Ledger> t;
            LedgerHistoryHelper h;
            t.insert(h["ab"]);
            t.insert(h["ac"]);
            t.insert(h["acf"]);
            t.insert(h["abde"], 2);

            BEAST_EXPECT(t.getPreferred(Seq{1})->id == h["ab"].id());
            BEAST_EXPECT(t.getPreferred(Seq{2})->id == h["ab"].id());
            BEAST_EXPECT(t.getPreferred(Seq{3})->id == h["a"].id());
            BEAST_EXPECT(t.getPreferred(Seq{4})->id == h["a"].id());

            
            t.remove(h["abde"]);
            t.insert(h["abdeg"]);

            BEAST_EXPECT(t.getPreferred(Seq{1})->id == h["ab"].id());
            BEAST_EXPECT(t.getPreferred(Seq{2})->id == h["ab"].id());
            BEAST_EXPECT(t.getPreferred(Seq{3})->id == h["a"].id());
            BEAST_EXPECT(t.getPreferred(Seq{4})->id == h["a"].id());
            BEAST_EXPECT(t.getPreferred(Seq{5})->id == h["a"].id());

            
            t.remove(h["ac"]);
            t.insert(h["abh"]);
            BEAST_EXPECT(t.getPreferred(Seq{1})->id == h["ab"].id());
            BEAST_EXPECT(t.getPreferred(Seq{2})->id == h["ab"].id());
            BEAST_EXPECT(t.getPreferred(Seq{3})->id == h["ab"].id());
            BEAST_EXPECT(t.getPreferred(Seq{4})->id == h["a"].id());
            BEAST_EXPECT(t.getPreferred(Seq{5})->id == h["a"].id());

            
            t.remove(h["acf"]);
            t.insert(h["abde"]);
            BEAST_EXPECT(t.getPreferred(Seq{1})->id == h["abde"].id());
            BEAST_EXPECT(t.getPreferred(Seq{2})->id == h["abde"].id());
            BEAST_EXPECT(t.getPreferred(Seq{3})->id == h["abde"].id());
            BEAST_EXPECT(t.getPreferred(Seq{4})->id == h["ab"].id());
            BEAST_EXPECT(t.getPreferred(Seq{5})->id == h["ab"].id());
        }
    }

    void
    testRootRelated()
    {
        using namespace csf;
        using Seq = Ledger::Seq;

        LedgerTrie<Ledger> t;
        LedgerHistoryHelper h;
        BEAST_EXPECT(!t.remove(h[""]));
        BEAST_EXPECT(t.branchSupport(h[""]) == 0);
        BEAST_EXPECT(t.tipSupport(h[""]) == 0);

        t.insert(h["a"]);
        BEAST_EXPECT(t.checkInvariants());
        BEAST_EXPECT(t.branchSupport(h[""]) == 1);
        BEAST_EXPECT(t.tipSupport(h[""]) == 0);

        t.insert(h["e"]);
        BEAST_EXPECT(t.checkInvariants());
        BEAST_EXPECT(t.branchSupport(h[""]) == 2);
        BEAST_EXPECT(t.tipSupport(h[""]) == 0);

        BEAST_EXPECT(t.remove(h["e"]));
        BEAST_EXPECT(t.checkInvariants());
        BEAST_EXPECT(t.branchSupport(h[""]) == 1);
        BEAST_EXPECT(t.tipSupport(h[""]) == 0);
    }

    void
    testStress()
    {
        using namespace csf;
        LedgerTrie<Ledger> t;
        LedgerHistoryHelper h;


        std::uint32_t const depth = 4;
        std::uint32_t const width = 4;

        std::uint32_t const iterations = 10000;

        std::mt19937 gen{42};
        std::uniform_int_distribution<> depthDist(0, depth - 1);
        std::uniform_int_distribution<> widthDist(0, width - 1);
        std::uniform_int_distribution<> flip(0, 1);
        for (std::uint32_t i = 0; i < iterations; ++i)
        {
            std::string curr = "";
            char depth = depthDist(gen);
            char offset = 0;
            for (char d = 0; d < depth; ++d)
            {
                char a = offset + widthDist(gen);
                curr += a;
                offset = (a + 1) * width;
            }

            if (flip(gen) == 0)
                t.insert(h[curr]);
            else
                t.remove(h[curr]);
            if (!BEAST_EXPECT(t.checkInvariants()))
                return;
        }
    }

    void
    run() override
    {
        testInsert();
        testRemove();
        testEmpty();
        testSupport();
        testGetPreferred();
        testRootRelated();
        testStress();
    }
};

BEAST_DEFINE_TESTSUITE(LedgerTrie, consensus, ripple);
}  
}  
























