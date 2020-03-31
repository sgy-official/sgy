

#ifndef RIPPLE_CONSENSUS_VALIDATIONS_H_INCLUDED
#define RIPPLE_CONSENSUS_VALIDATIONS_H_INCLUDED

#include <ripple/basics/Log.h>
#include <ripple/basics/UnorderedContainers.h>
#include <ripple/basics/chrono.h>
#include <ripple/beast/container/aged_container_utility.h>
#include <ripple/beast/container/aged_unordered_map.h>
#include <ripple/consensus/LedgerTrie.h>
#include <ripple/protocol/PublicKey.h>
#include <boost/optional.hpp>
#include <mutex>
#include <utility>
#include <vector>

namespace ripple {


struct ValidationParms
{
    explicit ValidationParms() = default;

    
    std::chrono::seconds validationCURRENT_WALL = std::chrono::minutes{5};

    
    std::chrono::seconds validationCURRENT_LOCAL = std::chrono::minutes{3};

    
    std::chrono::seconds validationCURRENT_EARLY = std::chrono::minutes{3};

    
    std::chrono::seconds validationSET_EXPIRES = std::chrono::minutes{10};

    
    std::chrono::seconds validationFRESHNESS = std::chrono::seconds {20};
};


template <class Seq>
class SeqEnforcer
{
    using time_point = std::chrono::steady_clock::time_point;
    Seq seq_{0};
    time_point when_;

public:
    
    bool
    operator()(time_point now, Seq s, ValidationParms const& p)
    {
        if (now > (when_ + p.validationSET_EXPIRES))
            seq_ = Seq{0};
        if (s <= seq_)
            return false;
        seq_ = s;
        when_ = now;
        return true;
    }

    Seq
    largest() const
    {
        return seq_;
    }
};

inline bool
isCurrent(
    ValidationParms const& p,
    NetClock::time_point now,
    NetClock::time_point signTime,
    NetClock::time_point seenTime)
{

    return (signTime > (now - p.validationCURRENT_EARLY)) &&
        (signTime < (now + p.validationCURRENT_WALL)) &&
        ((seenTime == NetClock::time_point{}) ||
         (seenTime < (now + p.validationCURRENT_LOCAL)));
}


enum class ValStatus {
    current,
    stale,
    badSeq
};

inline std::string
to_string(ValStatus m)
{
    switch (m)
    {
        case ValStatus::current:
            return "current";
        case ValStatus::stale:
            return "stale";
        case ValStatus::badSeq:
            return "badSeq";
        default:
            return "unknown";
    }
}


template <class Adaptor>
class Validations
{
    using Mutex = typename Adaptor::Mutex;
    using Validation = typename Adaptor::Validation;
    using Ledger = typename Adaptor::Ledger;
    using ID = typename Ledger::ID;
    using Seq = typename Ledger::Seq;
    using NodeID = typename Validation::NodeID;
    using NodeKey = typename Validation::NodeKey;

    using WrappedValidationType = std::decay_t<
        std::result_of_t<decltype (&Validation::unwrap)(Validation)>>;

    using ScopedLock = std::lock_guard<Mutex>;

    mutable Mutex mutex_;

    hash_map<NodeID, Validation> current_;

    SeqEnforcer<Seq> localSeqEnforcer_;

    hash_map<NodeID, SeqEnforcer<Seq>> seqEnforcers_;

    beast::aged_unordered_map<
        ID,
        hash_map<NodeID, Validation>,
        std::chrono::steady_clock,
        beast::uhash<>>
        byLedger_;

    LedgerTrie<Ledger> trie_;

    hash_map<NodeID, Ledger> lastLedger_;

    hash_map<std::pair<Seq, ID>, hash_set<NodeID>> acquiring_;

    ValidationParms const parms_;

    Adaptor adaptor_;

private:
    void
    removeTrie(ScopedLock const&, NodeID const& nodeID, Validation const& val)
    {
        {
            auto it =
                acquiring_.find(std::make_pair(val.seq(), val.ledgerID()));
            if (it != acquiring_.end())
            {
                it->second.erase(nodeID);
                if (it->second.empty())
                    acquiring_.erase(it);
            }
        }
        {
            auto it = lastLedger_.find(nodeID);
            if (it != lastLedger_.end() && it->second.id() == val.ledgerID())
            {
                trie_.remove(it->second);
                lastLedger_.erase(nodeID);
            }
        }
    }

    void
    checkAcquired(ScopedLock const& lock)
    {
        for (auto it = acquiring_.begin(); it != acquiring_.end();)
        {
            if (boost::optional<Ledger> ledger =
                    adaptor_.acquire(it->first.second))
            {
                for (NodeID const& nodeID : it->second)
                    updateTrie(lock, nodeID, *ledger);

                it = acquiring_.erase(it);
            }
            else
                ++it;
        }
    }

    void
    updateTrie(ScopedLock const&, NodeID const& nodeID, Ledger ledger)
    {
        auto ins = lastLedger_.emplace(nodeID, ledger);
        if (!ins.second)
        {
            trie_.remove(ins.first->second);
            ins.first->second = ledger;
        }
        trie_.insert(ledger);
    }

    
    void
    updateTrie(
        ScopedLock const& lock,
        NodeID const& nodeID,
        Validation const& val,
        boost::optional<std::pair<Seq, ID>> prior)
    {
        assert(val.trusted());

        if (prior)
        {
            auto it = acquiring_.find(*prior);
            if (it != acquiring_.end())
            {
                it->second.erase(nodeID);
                if (it->second.empty())
                    acquiring_.erase(it);
            }
        }

        checkAcquired(lock);

        std::pair<Seq, ID> valPair{val.seq(), val.ledgerID()};
        auto it = acquiring_.find(valPair);
        if (it != acquiring_.end())
        {
            it->second.insert(nodeID);
        }
        else
        {
            if (boost::optional<Ledger> ledger =
                    adaptor_.acquire(val.ledgerID()))
                updateTrie(lock, nodeID, *ledger);
            else
                acquiring_[valPair].insert(nodeID);
        }
    }

    
    template <class F>
    auto
    withTrie(ScopedLock const& lock, F&& f)
    {
        current(lock, [](auto) {}, [](auto, auto) {});
        checkAcquired(lock);
        return f(trie_);
    }

    

    template <class Pre, class F>
    void
    current(ScopedLock const& lock, Pre&& pre, F&& f)
    {
        NetClock::time_point t = adaptor_.now();
        pre(current_.size());
        auto it = current_.begin();
        while (it != current_.end())
        {
            if (!isCurrent(
                    parms_, t, it->second.signTime(), it->second.seenTime()))
            {
                removeTrie(lock, it->first, it->second);
                adaptor_.onStale(std::move(it->second));
                it = current_.erase(it);
            }
            else
            {
                auto cit = typename decltype(current_)::const_iterator{it};
                f(cit->first, cit->second);
                ++it;
            }
        }
    }

    
    template <class Pre, class F>
    void
    byLedger(ScopedLock const&, ID const& ledgerID, Pre&& pre, F&& f)
    {
        auto it = byLedger_.find(ledgerID);
        if (it != byLedger_.end())
        {
            byLedger_.touch(it);
            pre(it->second.size());
            for (auto const& keyVal : it->second)
                f(keyVal.first, keyVal.second);
        }
    }

public:
    
    template <class... Ts>
    Validations(
        ValidationParms const& p,
        beast::abstract_clock<std::chrono::steady_clock>& c,
        Ts&&... ts)
        : byLedger_(c), parms_(p), adaptor_(std::forward<Ts>(ts)...)
    {
    }

    
    Adaptor const&
    adaptor() const
    {
        return adaptor_;
    }

    
    ValidationParms const&
    parms() const
    {
        return parms_;
    }

    
    bool
    canValidateSeq(Seq const s)
    {
        ScopedLock lock{mutex_};
        return localSeqEnforcer_(byLedger_.clock().now(), s, parms_);
    }

    
    ValStatus
    add(NodeID const& nodeID, Validation const& val)
    {
        if (!isCurrent(parms_, adaptor_.now(), val.signTime(), val.seenTime()))
            return ValStatus::stale;

        {
            ScopedLock lock{mutex_};

            auto const now = byLedger_.clock().now();
            SeqEnforcer<Seq>& enforcer = seqEnforcers_[nodeID];
            if (!enforcer(now, val.seq(), parms_))
                return ValStatus::badSeq;

            auto byLedgerIt = byLedger_[val.ledgerID()].emplace(nodeID, val);
            if (!byLedgerIt.second)
                byLedgerIt.first->second = val;

            auto const ins = current_.emplace(nodeID, val);
            if (!ins.second)
            {
                Validation& oldVal = ins.first->second;
                if (val.signTime() > oldVal.signTime())
                {
                    std::pair<Seq, ID> old(oldVal.seq(), oldVal.ledgerID());
                    adaptor_.onStale(std::move(oldVal));
                    ins.first->second = val;
                    if (val.trusted())
                        updateTrie(lock, nodeID, val, old);
                }
                else
                    return ValStatus::stale;
            }
            else if (val.trusted())
            {
                updateTrie(lock, nodeID, val, boost::none);
            }
        }
        return ValStatus::current;
    }

    
    void
    expire()
    {
        ScopedLock lock{mutex_};
        beast::expire(byLedger_, parms_.validationSET_EXPIRES);
    }

    
    void
    trustChanged(hash_set<NodeID> const& added, hash_set<NodeID> const& removed)
    {
        ScopedLock lock{mutex_};

        for (auto& it : current_)
        {
            if (added.find(it.first) != added.end())
            {
                it.second.setTrusted();
                updateTrie(lock, it.first, it.second, boost::none);
            }
            else if (removed.find(it.first) != removed.end())
            {
                it.second.setUntrusted();
                removeTrie(lock, it.first, it.second);
            }
        }

        for (auto& it : byLedger_)
        {
            for (auto& nodeVal : it.second)
            {
                if (added.find(nodeVal.first) != added.end())
                {
                    nodeVal.second.setTrusted();
                }
                else if (removed.find(nodeVal.first) != removed.end())
                {
                    nodeVal.second.setUntrusted();
                }
            }
        }
    }

    Json::Value
    getJsonTrie() const
    {
        ScopedLock lock{mutex_};
        return trie_.getJson();
    }

    
    boost::optional<std::pair<Seq, ID>>
    getPreferred(Ledger const& curr)
    {
        ScopedLock lock{mutex_};
        boost::optional<SpanTip<Ledger>> preferred =
            withTrie(lock, [this](LedgerTrie<Ledger>& trie) {
                return trie.getPreferred(localSeqEnforcer_.largest());
            });
        if (!preferred)
        {
            auto it = std::max_element(
                acquiring_.begin(),
                acquiring_.end(),
                [](auto const& a, auto const& b) {
                    std::pair<Seq, ID> const& aKey = a.first;
                    typename hash_set<NodeID>::size_type const& aSize =
                        a.second.size();
                    std::pair<Seq, ID> const& bKey = b.first;
                    typename hash_set<NodeID>::size_type const& bSize =
                        b.second.size();
                    return std::tie(aSize, aKey.second) <
                        std::tie(bSize, bKey.second);
                });
            if (it != acquiring_.end())
                return it->first;
            return boost::none;
        }

        if (preferred->seq == curr.seq() + Seq{1} &&
            preferred->ancestor(curr.seq()) == curr.id())
            return std::make_pair(curr.seq(), curr.id());

        if (preferred->seq > curr.seq())
            return std::make_pair(preferred->seq, preferred->id);

        if (curr[preferred->seq] != preferred->id)
            return std::make_pair(preferred->seq, preferred->id);

        return std::make_pair(curr.seq(), curr.id());
    }

    
    ID
    getPreferred(Ledger const& curr, Seq minValidSeq)
    {
        boost::optional<std::pair<Seq, ID>> preferred = getPreferred(curr);
        if (preferred && preferred->first >= minValidSeq)
            return preferred->second;
        return curr.id();
    }

    
    ID
    getPreferredLCL(
        Ledger const& lcl,
        Seq minSeq,
        hash_map<ID, std::uint32_t> const& peerCounts)
    {
        boost::optional<std::pair<Seq, ID>> preferred = getPreferred(lcl);

        if (preferred)
            return (preferred->first >= minSeq) ? preferred->second : lcl.id();

        auto it = std::max_element(
            peerCounts.begin(), peerCounts.end(), [](auto& a, auto& b) {
                return std::tie(a.second, a.first) <
                    std::tie(b.second, b.first);
            });

        if (it != peerCounts.end())
            return it->first;
        return lcl.id();
    }

    
    std::size_t
    getNodesAfter(Ledger const& ledger, ID const& ledgerID)
    {
        ScopedLock lock{mutex_};

        if (ledger.id() == ledgerID)
            return withTrie(lock, [&ledger](LedgerTrie<Ledger>& trie) {
                return trie.branchSupport(ledger) - trie.tipSupport(ledger);
            });

        return std::count_if(
            lastLedger_.begin(),
            lastLedger_.end(),
            [&ledgerID](auto const& it) {
                auto const& curr = it.second;
                return curr.seq() > Seq{0} &&
                    curr[curr.seq() - Seq{1}] == ledgerID;
            });
    }

    
    std::vector<WrappedValidationType>
    currentTrusted()
    {
        std::vector<WrappedValidationType> ret;
        ScopedLock lock{mutex_};
        current(
            lock,
            [&](std::size_t numValidations) { ret.reserve(numValidations); },
            [&](NodeID const&, Validation const& v) {
                if (v.trusted() && v.full())
                    ret.push_back(v.unwrap());
            });
        return ret;
    }

    
    auto
    getCurrentNodeIDs() -> hash_set<NodeID>
    {
        hash_set<NodeID> ret;
        ScopedLock lock{mutex_};
        current(
            lock,
            [&](std::size_t numValidations) { ret.reserve(numValidations); },
            [&](NodeID const& nid, Validation const&) { ret.insert(nid); });

        return ret;
    }

    
    std::size_t
    numTrustedForLedger(ID const& ledgerID)
    {
        std::size_t count = 0;
        ScopedLock lock{mutex_};
        byLedger(
            lock,
            ledgerID,
            [&](std::size_t) {},  
            [&](NodeID const&, Validation const& v) {
                if (v.trusted() && v.full())
                    ++count;
            });
        return count;
    }

    
    std::vector<WrappedValidationType>
    getTrustedForLedger(ID const& ledgerID)
    {
        std::vector<WrappedValidationType> res;
        ScopedLock lock{mutex_};
        byLedger(
            lock,
            ledgerID,
            [&](std::size_t numValidations) { res.reserve(numValidations); },
            [&](NodeID const&, Validation const& v) {
                if (v.trusted() && v.full())
                    res.emplace_back(v.unwrap());
            });

        return res;
    }

    
    std::vector<std::uint32_t>
    fees(ID const& ledgerID, std::uint32_t baseFee)
    {
        std::vector<std::uint32_t> res;
        ScopedLock lock{mutex_};
        byLedger(
            lock,
            ledgerID,
            [&](std::size_t numValidations) { res.reserve(numValidations); },
            [&](NodeID const&, Validation const& v) {
                if (v.trusted() && v.full())
                {
                    boost::optional<std::uint32_t> loadFee = v.loadFee();
                    if (loadFee)
                        res.push_back(*loadFee);
                    else
                        res.push_back(baseFee);
                }
            });
        return res;
    }

    
    void
    flush()
    {
        hash_map<NodeID, Validation> flushed;
        {
            ScopedLock lock{mutex_};
            for (auto it : current_)
            {
                flushed.emplace(it.first, std::move(it.second));
            }
            current_.clear();
        }

        adaptor_.flush(std::move(flushed));
    }

    
    std::size_t
    laggards(Seq const seq, hash_set<NodeKey>& trustedKeys)
    {
        std::size_t laggards = 0;

        current(ScopedLock{mutex_},
            [](std::size_t) {},
            [&](NodeID const&, Validation const& v) {
                if (adaptor_.now() <
                        v.seenTime() + parms_.validationFRESHNESS &&
                    trustedKeys.find(v.key()) != trustedKeys.end())
                {
                    trustedKeys.erase(v.key());
                    if (seq > v.seq())
                        ++laggards;
                }
            });

        return laggards;
    }
};

}  
#endif








