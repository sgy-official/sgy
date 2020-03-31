

#ifndef RIPPLE_APP_CONSENSUS_LEDGERS_TRIE_H_INCLUDED
#define RIPPLE_APP_CONSENSUS_LEDGERS_TRIE_H_INCLUDED

#include <ripple/json/json_value.h>
#include <boost/optional.hpp>
#include <algorithm>
#include <memory>
#include <sstream>
#include <stack>
#include <vector>

namespace ripple {


template <class Ledger>
class SpanTip
{
public:
    using Seq = typename Ledger::Seq;
    using ID = typename Ledger::ID;

    SpanTip(Seq s, ID i, Ledger const lgr)
        : seq{s}, id{i}, ledger{std::move(lgr)}
    {
    }

    Seq seq;
    ID id;

    
    ID
    ancestor(Seq const& s) const
    {
        assert(s <= seq);
        return ledger[s];
    }

private:
    Ledger const ledger;
};

namespace ledger_trie_detail {

template <class Ledger>
class Span
{
    using Seq = typename Ledger::Seq;
    using ID = typename Ledger::ID;

    Seq start_{0};
    Seq end_{1};
    Ledger ledger_;

public:
    Span() : ledger_{typename Ledger::MakeGenesis{}}
    {
        assert(ledger_.seq() == start_);
    }

    Span(Ledger ledger)
        : start_{0}, end_{ledger.seq() + Seq{1}}, ledger_{std::move(ledger)}
    {
    }

    Span(Span const& s) = default;
    Span(Span&& s) = default;
    Span&
    operator=(Span const&) = default;
    Span&
    operator=(Span&&) = default;

    Seq
    start() const
    {
        return start_;
    }

    Seq
    end() const
    {
        return end_;
    }

    boost::optional<Span>
    from(Seq spot) const
    {
        return sub(spot, end_);
    }

    boost::optional<Span>
    before(Seq spot) const
    {
        return sub(start_, spot);
    }

    ID
    startID() const
    {
        return ledger_[start_];
    }

    Seq
    diff(Ledger const& o) const
    {
        return clamp(mismatch(ledger_, o));
    }

    SpanTip<Ledger>
    tip() const
    {
        Seq tipSeq{end_ - Seq{1}};
        return SpanTip<Ledger>{tipSeq, ledger_[tipSeq], ledger_};
    }

private:
    Span(Seq start, Seq end, Ledger const& l)
        : start_{start}, end_{end}, ledger_{l}
    {
        assert(start < end);
    }

    Seq
    clamp(Seq val) const
    {
        return std::min(std::max(start_, val), end_);
    }

    boost::optional<Span>
    sub(Seq from, Seq to) const
    {
        Seq newFrom = clamp(from);
        Seq newTo = clamp(to);
        if (newFrom < newTo)
            return Span(newFrom, newTo, ledger_);
        return boost::none;
    }

    friend std::ostream&
    operator<<(std::ostream& o, Span const& s)
    {
        return o << s.tip().id << "[" << s.start_ << "," << s.end_ << ")";
    }

    friend Span
    merge(Span const& a, Span const& b)
    {
        if (a.end_ < b.end_)
            return Span(std::min(a.start_, b.start_), b.end_, b.ledger_);

        return Span(std::min(a.start_, b.start_), a.end_, a.ledger_);
    }
};

template <class Ledger>
struct Node
{
    Node() = default;

    explicit Node(Ledger const& l) : span{l}, tipSupport{1}, branchSupport{1}
    {
    }

    explicit Node(Span<Ledger> s) : span{std::move(s)}
    {
    }

    Span<Ledger> span;
    std::uint32_t tipSupport = 0;
    std::uint32_t branchSupport = 0;

    std::vector<std::unique_ptr<Node>> children;
    Node* parent = nullptr;

    
    void
    erase(Node const* child)
    {
        auto it = std::find_if(
            children.begin(),
            children.end(),
            [child](std::unique_ptr<Node> const& curr) {
                return curr.get() == child;
            });
        assert(it != children.end());
        std::swap(*it, children.back());
        children.pop_back();
    }

    friend std::ostream&
    operator<<(std::ostream& o, Node const& s)
    {
        return o << s.span << "(T:" << s.tipSupport << ",B:" << s.branchSupport
                 << ")";
    }

    Json::Value
    getJson() const
    {
        Json::Value res;
        std::stringstream sps;
        sps << span;
        res["span"] = sps.str();
        res["startID"] = to_string(span.startID());
        res["seq"] = static_cast<std::uint32_t>(span.tip().seq);
        res["tipSupport"] = tipSupport;
        res["branchSupport"] = branchSupport;
        if (!children.empty())
        {
            Json::Value& cs = (res["children"] = Json::arrayValue);
            for (auto const& child : children)
            {
                cs.append(child->getJson());
            }
        }
        return res;
    }
};
}  


template <class Ledger>
class LedgerTrie
{
    using Seq = typename Ledger::Seq;
    using ID = typename Ledger::ID;

    using Node = ledger_trie_detail::Node<Ledger>;
    using Span = ledger_trie_detail::Span<Ledger>;

    std::unique_ptr<Node> root;

    std::map<Seq, std::uint32_t> seqSupport;

    
    std::pair<Node*, Seq>
    find(Ledger const& ledger) const
    {
        Node* curr = root.get();

        assert(curr);
        Seq pos = curr->span.diff(ledger);

        bool done = false;

        while (!done && pos == curr->span.end())
        {
            done = true;
            for (std::unique_ptr<Node> const& child : curr->children)
            {
                auto const childPos = child->span.diff(ledger);
                if (childPos > pos)
                {
                    done = false;
                    pos = childPos;
                    curr = child.get();
                    break;
                }
            }
        }
        return std::make_pair(curr, pos);
    }

    
    Node*
    findByLedgerID(Ledger const& ledger, Node* parent = nullptr) const
    {
        if (!parent)
            parent = root.get();
        if (ledger.id() == parent->span.tip().id)
            return parent;
        for (auto const& child : parent->children)
        {
            auto cl = findByLedgerID(ledger, child.get());
            if (cl)
                return cl;
        }
        return nullptr;
    }

    void
    dumpImpl(std::ostream& o, std::unique_ptr<Node> const& curr, int offset)
        const
    {
        if (curr)
        {
            if (offset > 0)
                o << std::setw(offset) << "|-";

            std::stringstream ss;
            ss << *curr;
            o << ss.str() << std::endl;
            for (std::unique_ptr<Node> const& child : curr->children)
                dumpImpl(o, child, offset + 1 + ss.str().size() + 2);
        }
    }

public:
    LedgerTrie() : root{std::make_unique<Node>()}
    {
    }

    
    void
    insert(Ledger const& ledger, std::uint32_t count = 1)
    {
        Node* loc;
        Seq diffSeq;
        std::tie(loc, diffSeq) = find(ledger);

        assert(loc);

        Node* incNode = loc;


        boost::optional<Span> prefix = loc->span.before(diffSeq);
        boost::optional<Span> oldSuffix = loc->span.from(diffSeq);
        boost::optional<Span> newSuffix = Span{ledger}.from(diffSeq);

        if (oldSuffix)
        {

            auto newNode = std::make_unique<Node>(*oldSuffix);
            newNode->tipSupport = loc->tipSupport;
            newNode->branchSupport = loc->branchSupport;
            newNode->children = std::move(loc->children);
            assert(loc->children.empty());
            for (std::unique_ptr<Node>& child : newNode->children)
                child->parent = newNode.get();

            assert(prefix);
            loc->span = *prefix;
            newNode->parent = loc;
            loc->children.emplace_back(std::move(newNode));
            loc->tipSupport = 0;
        }
        if (newSuffix)
        {

            auto newNode = std::make_unique<Node>(*newSuffix);
            newNode->parent = loc;
            incNode = newNode.get();
            loc->children.push_back(std::move(newNode));
        }

        incNode->tipSupport += count;
        while (incNode)
        {
            incNode->branchSupport += count;
            incNode = incNode->parent;
        }

        seqSupport[ledger.seq()] += count;
    }

    
    bool
    remove(Ledger const& ledger, std::uint32_t count = 1)
    {
        Node* loc = findByLedgerID(ledger);
        if (!loc || loc->tipSupport == 0)
            return false;

        count = std::min(count, loc->tipSupport);
        loc->tipSupport -= count;

        auto const it = seqSupport.find(ledger.seq());
        assert(it != seqSupport.end() && it->second >= count);
        it->second -= count;
        if (it->second == 0)
            seqSupport.erase(it->first);

        Node* decNode = loc;
        while (decNode)
        {
            decNode->branchSupport -= count;
            decNode = decNode->parent;
        }

        while (loc->tipSupport == 0 && loc != root.get())
        {
            Node* parent = loc->parent;
            if (loc->children.empty())
            {
                parent->erase(loc);
            }
            else if (loc->children.size() == 1)
            {
                std::unique_ptr<Node> child =
                    std::move(loc->children.front());
                child->span = merge(loc->span, child->span);
                child->parent = parent;
                parent->children.emplace_back(std::move(child));
                parent->erase(loc);
            }
            else
                break;
            loc = parent;
        }
        return true;
    }

    
    std::uint32_t
    tipSupport(Ledger const& ledger) const
    {
        if (auto const* loc = findByLedgerID(ledger))
            return loc->tipSupport;
        return 0;
    }

    
    std::uint32_t
    branchSupport(Ledger const& ledger) const
    {
        Node const* loc = findByLedgerID(ledger);
        if (!loc)
        {
            Seq diffSeq;
            std::tie(loc, diffSeq) = find(ledger);
            if (! (diffSeq > ledger.seq() && ledger.seq() < loc->span.end()))
                loc = nullptr;
        }
        return loc ? loc->branchSupport : 0;
    }

    
    boost::optional<SpanTip<Ledger>>
    getPreferred(Seq const largestIssued) const
    {
        if (empty())
            return boost::none;

        Node* curr = root.get();

        bool done = false;

        std::uint32_t uncommitted = 0;
        auto uncommittedIt = seqSupport.begin();

        while (curr && !done)
        {
            {
                Seq nextSeq = curr->span.start() + Seq{1};
                while (uncommittedIt != seqSupport.end() &&
                       uncommittedIt->first < std::max(nextSeq, largestIssued))
                {
                    uncommitted += uncommittedIt->second;
                    uncommittedIt++;
                }

                while (nextSeq < curr->span.end() &&
                       curr->branchSupport > uncommitted)
                {
                    if (uncommittedIt != seqSupport.end() &&
                        uncommittedIt->first < curr->span.end())
                    {
                        nextSeq = uncommittedIt->first + Seq{1};
                        uncommitted += uncommittedIt->second;
                        uncommittedIt++;
                    }
                    else  
                        nextSeq = curr->span.end();
                }
                if (nextSeq < curr->span.end())
                    return curr->span.before(nextSeq)->tip();
            }

            Node* best = nullptr;
            std::uint32_t margin = 0;
            if (curr->children.size() == 1)
            {
                best = curr->children[0].get();
                margin = best->branchSupport;
            }
            else if (!curr->children.empty())
            {
                std::partial_sort(
                    curr->children.begin(),
                    curr->children.begin() + 2,
                    curr->children.end(),
                    [](std::unique_ptr<Node> const& a,
                       std::unique_ptr<Node> const& b) {
                        return std::make_tuple(
                                   a->branchSupport, a->span.startID()) >
                            std::make_tuple(
                                   b->branchSupport, b->span.startID());
                    });

                best = curr->children[0].get();
                margin = curr->children[0]->branchSupport -
                    curr->children[1]->branchSupport;

                if (best->span.startID() > curr->children[1]->span.startID())
                    margin++;
            }

            if (best && ((margin > uncommitted) || (uncommitted == 0)))
                curr = best;
            else  
                done = true;
        }
        return curr->span.tip();
    }

    
    bool
    empty() const
    {
        return !root || root->branchSupport == 0;
    }

    
    void
    dump(std::ostream& o) const
    {
        dumpImpl(o, root, 0);
    }

    
    Json::Value
    getJson() const
    {
        Json::Value res;
        res["trie"] = root->getJson();
        res["seq_support"] = Json::objectValue;
        for (auto const& mit : seqSupport)
            res["seq_support"][to_string(mit.first)] = mit.second;
        return res;
    }

    
    bool
    checkInvariants() const
    {
        std::map<Seq, std::uint32_t> expectedSeqSupport;

        std::stack<Node const*> nodes;
        nodes.push(root.get());
        while (!nodes.empty())
        {
            Node const* curr = nodes.top();
            nodes.pop();
            if (!curr)
                continue;

            if (curr != root.get() && curr->tipSupport == 0 &&
                curr->children.size() < 2)
                return false;

            std::size_t support = curr->tipSupport;
            if (curr->tipSupport != 0)
                expectedSeqSupport[curr->span.end() - Seq{1}] +=
                    curr->tipSupport;

            for (auto const& child : curr->children)
            {
                if (child->parent != curr)
                    return false;

                support += child->branchSupport;
                nodes.push(child.get());
            }
            if (support != curr->branchSupport)
                return false;
        }
        return expectedSeqSupport == seqSupport;
    }
};

}  
#endif








