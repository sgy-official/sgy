

#include <ripple/app/consensus/RCLValidations.h>
#include <ripple/app/ledger/Ledger.h>
#include <ripple/basics/Log.h>
#include <ripple/ledger/View.h>
#include <ripple/beast/unit_test.h>
#include <test/jtx.h>

namespace ripple {
namespace test {

class RCLValidations_test : public beast::unit_test::suite
{

    void
    testChangeTrusted()
    {
        testcase("Change validation trusted status");
        auto keys = randomKeyPair(KeyType::secp256k1);
        auto v = std::make_shared<STValidation>(
            uint256(),
            1,
            uint256(),
            NetClock::time_point(),
            keys.first,
            keys.second,
            calcNodeID(keys.first),
            true,
            STValidation::FeeSettings{},
            std::vector<uint256>{});

        BEAST_EXPECT(v->isTrusted());
        v->setUntrusted();
        BEAST_EXPECT(!v->isTrusted());

        RCLValidation rcv{v};
        BEAST_EXPECT(!rcv.trusted());
        rcv.setTrusted();
        BEAST_EXPECT(rcv.trusted());
        rcv.setUntrusted();
        BEAST_EXPECT(!rcv.trusted());
    }

    void
    testRCLValidatedLedger()
    {
        testcase("RCLValidatedLedger ancestry");

        using Seq = RCLValidatedLedger::Seq;
        using ID = RCLValidatedLedger::ID;


        Seq const maxAncestors = 256;


        std::vector<std::shared_ptr<Ledger const>> history;

        jtx::Env env(*this);
        Config config;
        auto prev = std::make_shared<Ledger const>(
            create_genesis, config,
            std::vector<uint256>{}, env.app().family());
        history.push_back(prev);
        for (auto i = 0; i < (2*maxAncestors + 1); ++i)
        {
            auto next = std::make_shared<Ledger>(
                *prev,
                env.app().timeKeeper().closeTime());
            next->updateSkipList();
            history.push_back(next);
            prev = next;
        }

        Seq const diverge = history.size()/2;
        std::vector<std::shared_ptr<Ledger const>> altHistory(
            history.begin(), history.begin() + diverge);
        using namespace std::chrono_literals;
        env.timeKeeper().set(env.timeKeeper().now() + 1200s);
        prev = altHistory.back();
        bool forceHash = true;
        while(altHistory.size() < history.size())
        {
            auto next = std::make_shared<Ledger>(
                *prev,
                env.app().timeKeeper().closeTime());
            next->updateSkipList();
            if(forceHash)
            {
                next->setImmutable(config);
                forceHash = false;
            }

            altHistory.push_back(next);
            prev = next;
        }



        {
            RCLValidatedLedger a{RCLValidatedLedger::MakeGenesis{}};
            BEAST_EXPECT(a.seq() == Seq{0});
            BEAST_EXPECT(a[Seq{0}] == ID{0});
            BEAST_EXPECT(a.minSeq() == Seq{0});
        }

        {
            std::shared_ptr<Ledger const> ledger = history.back();
            RCLValidatedLedger a{ledger, env.journal};
            BEAST_EXPECT(a.seq() == ledger->info().seq);
            BEAST_EXPECT(
                a.minSeq() == a.seq() - maxAncestors);
            for(Seq s = a.seq(); s > 0; s--)
            {
                if(s >= a.minSeq())
                    BEAST_EXPECT(a[s] == history[s-1]->info().hash);
                else
                    BEAST_EXPECT(a[s] == ID{0});
            }
        }


        {
            RCLValidatedLedger a{RCLValidatedLedger::MakeGenesis{}};

            for (auto ledger : {history.back(),
                                history[maxAncestors - 1]})
            {
                RCLValidatedLedger b{ledger, env.journal};
                BEAST_EXPECT(mismatch(a, b) == 1);
                BEAST_EXPECT(mismatch(b, a) == 1);
            }
        }
        {
            RCLValidatedLedger a{history.back(), env.journal};
            for(Seq s = a.seq(); s > 0; s--)
            {
                RCLValidatedLedger b{history[s-1], env.journal};
                if(s >= a.minSeq())
                {
                    BEAST_EXPECT(mismatch(a, b) == b.seq() + 1);
                    BEAST_EXPECT(mismatch(b, a) == b.seq() + 1);
                }
                else
                {
                    BEAST_EXPECT(mismatch(a, b) == Seq{1});
                    BEAST_EXPECT(mismatch(b, a) == Seq{1});
                }
            }

        }
        {
            for(Seq s = 1; s < history.size(); ++s)
            {
                RCLValidatedLedger a{history[s-1], env.journal};
                RCLValidatedLedger b{altHistory[s-1], env.journal};

                BEAST_EXPECT(a.seq() == b.seq());
                if(s <= diverge)
                {
                    BEAST_EXPECT(a[a.seq()] == b[b.seq()]);
                    BEAST_EXPECT(mismatch(a,b) == a.seq() + 1);
                    BEAST_EXPECT(mismatch(b,a) == a.seq() + 1);
                }
                else
                {
                    BEAST_EXPECT(a[a.seq()] != b[b.seq()]);
                    BEAST_EXPECT(mismatch(a,b) == diverge + 1);
                    BEAST_EXPECT(mismatch(b,a) == diverge + 1);
                }
            }
        }
        {
            RCLValidatedLedger a{history[diverge], env.journal};
            for(Seq offset = diverge/2; offset < 3*diverge/2; ++offset)
            {
                RCLValidatedLedger b{altHistory[offset-1], env.journal};
                if(offset <= diverge)
                {
                    BEAST_EXPECT(mismatch(a,b) == b.seq() + 1);
                }
                else
                {
                    BEAST_EXPECT(mismatch(a,b) == diverge + 1);
                }
            }
        }
    }

    void
    testLedgerTrieRCLValidatedLedger()
    {
        testcase("RCLValidatedLedger LedgerTrie");


        using Seq = RCLValidatedLedger::Seq;
        using ID = RCLValidatedLedger::ID;

        Seq const maxAncestors = 256;
        std::vector<std::shared_ptr<Ledger const>> history;

        jtx::Env env(*this);
        auto& j = env.journal;
        Config config;
        auto prev = std::make_shared<Ledger const>(
            create_genesis, config, std::vector<uint256>{}, env.app().family());
        history.push_back(prev);
        for (auto i = 0; i < (maxAncestors + 10); ++i)
        {
            auto next = std::make_shared<Ledger>(
                *prev, env.app().timeKeeper().closeTime());
            next->updateSkipList();
            history.push_back(next);
            prev = next;
        }

        LedgerTrie<RCLValidatedLedger> trie;

        auto ledg_002 = RCLValidatedLedger{history[1], j};
        auto ledg_258 = RCLValidatedLedger{history[257], j};
        auto ledg_259 = RCLValidatedLedger{history[258], j};

        trie.insert(ledg_002);
        trie.insert(ledg_258, 4);
        BEAST_EXPECT(trie.tipSupport(ledg_002) == 1);
        BEAST_EXPECT(trie.branchSupport(ledg_002) == 5);
        BEAST_EXPECT(trie.tipSupport(ledg_258) == 4);
        BEAST_EXPECT(trie.branchSupport(ledg_258) == 4);

        BEAST_EXPECT(
            trie.remove(ledg_258, 3));
        trie.insert(ledg_259, 3);
        trie.getPreferred(1);
        BEAST_EXPECT(trie.tipSupport(ledg_002) == 1);
        BEAST_EXPECT(trie.branchSupport(ledg_002) == 2);
        BEAST_EXPECT(trie.tipSupport(ledg_258) == 1);
        BEAST_EXPECT(trie.branchSupport(ledg_258) == 1);
        BEAST_EXPECT(trie.tipSupport(ledg_259) == 3);
        BEAST_EXPECT(trie.branchSupport(ledg_259) == 3);


        BEAST_EXPECT(
            trie.remove(RCLValidatedLedger{history[257], env.journal}, 1));
        trie.insert(RCLValidatedLedger{history[258], env.journal}, 1);
        trie.getPreferred(1);
        BEAST_EXPECT(trie.tipSupport(ledg_002) == 1);
        BEAST_EXPECT(trie.branchSupport(ledg_002) == 1);
        BEAST_EXPECT(trie.tipSupport(ledg_258) == 0);
        BEAST_EXPECT(trie.branchSupport(ledg_258) == 4);
        BEAST_EXPECT(trie.tipSupport(ledg_259) == 4);
        BEAST_EXPECT(trie.branchSupport(ledg_259) == 4);
    }

public:
    void
    run() override
    {
        testChangeTrusted();
        testRCLValidatedLedger();
        testLedgerTrieRCLValidatedLedger();
    }
};

BEAST_DEFINE_TESTSUITE(RCLValidations, app, ripple);

}  
}  
























