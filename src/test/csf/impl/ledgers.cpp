
#include <test/csf/ledgers.h>
#include <algorithm>

#include <sstream>

namespace ripple {
namespace test {
namespace csf {

Ledger::Instance const Ledger::genesis;

Json::Value
Ledger::getJson() const
{
    Json::Value res(Json::objectValue);
    res["id"] = static_cast<ID::value_type>(id());
    res["seq"] = static_cast<Seq::value_type>(seq());
    return res;
}

bool
Ledger::isAncestor(Ledger const& ancestor) const
{
    if (ancestor.seq() < seq())
        return operator[](ancestor.seq()) == ancestor.id();
    return false;
}

Ledger::ID
Ledger::operator[](Seq s) const
{
    if(s > seq())
        return {};
    if(s== seq())
        return id();
    return instance_->ancestors[static_cast<Seq::value_type>(s)];

}

Ledger::Seq
mismatch(Ledger const& a, Ledger const& b)
{
    using Seq = Ledger::Seq;

    Seq start{0};
    Seq end = std::min(a.seq() + Seq{1}, b.seq() + Seq{1});

    Seq count = end - start;
    while(count > Seq{0})
    {
        Seq step = count/Seq{2};
        Seq curr = start + step;
        if(a[curr] == b[curr])
        {
            start = ++curr;
            count -= step + Seq{1};
        }
        else
            count = step;
    }
    return start;
}

LedgerOracle::LedgerOracle()
{
    instances_.insert(InstanceEntry{Ledger::genesis, nextID()});
}

Ledger::ID
LedgerOracle::nextID() const
{
    return Ledger::ID{static_cast<Ledger::ID::value_type>(instances_.size())};
}

Ledger
LedgerOracle::accept(
    Ledger const& parent,
    TxSetType const& txs,
    NetClock::duration closeTimeResolution,
    NetClock::time_point const& consensusCloseTime)
{
    using namespace std::chrono_literals;
    Ledger::Instance next(*parent.instance_);
    next.txs.insert(txs.begin(), txs.end());
    next.seq = parent.seq() + Ledger::Seq{1};
    next.closeTimeResolution = closeTimeResolution;
    next.closeTimeAgree = consensusCloseTime != NetClock::time_point{};
    if(next.closeTimeAgree)
        next.closeTime = effCloseTime(
            consensusCloseTime, closeTimeResolution, parent.closeTime());
    else
        next.closeTime = parent.closeTime() + 1s;

    next.parentCloseTime = parent.closeTime();
    next.parentID = parent.id();
    next.ancestors.push_back(parent.id());

    auto it = instances_.left.find(next);
    if (it == instances_.left.end())
    {
        using Entry = InstanceMap::left_value_type;
        it = instances_.left.insert(Entry{next, nextID()}).first;
    }
    return Ledger(it->second, &(it->first));
}

boost::optional<Ledger>
LedgerOracle::lookup(Ledger::ID const & id) const
{
    auto const it = instances_.right.find(id);
    if(it != instances_.right.end())
    {
        return Ledger(it->first, &(it->second));
    }
    return boost::none;
}


std::size_t
LedgerOracle::branches(std::set<Ledger> const & ledgers) const
{
    std::vector<Ledger> tips;
    tips.reserve(ledgers.size());

    for (Ledger const & ledger : ledgers)
    {
        bool found = false;
        for (auto idx = 0; idx < tips.size() && !found; ++idx)
        {
            bool const idxEarlier = tips[idx].seq() < ledger.seq();
            Ledger const & earlier = idxEarlier ? tips[idx] : ledger;
            Ledger const & later = idxEarlier ? ledger : tips[idx] ;
            if (later.isAncestor(earlier))
            {
                tips[idx] = later;
                found = true;
            }
        }

        if(!found)
            tips.push_back(ledger);

    }
    return tips.size();
}
}  
}  
}  
























