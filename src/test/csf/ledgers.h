
#ifndef RIPPLE_TEST_CSF_LEDGERS_H_INCLUDED
#define RIPPLE_TEST_CSF_LEDGERS_H_INCLUDED

#include <ripple/basics/UnorderedContainers.h>
#include <ripple/basics/chrono.h>
#include <ripple/consensus/LedgerTiming.h>
#include <ripple/basics/tagged_integer.h>
#include <ripple/consensus/LedgerTiming.h>
#include <ripple/json/json_value.h>
#include <test/csf/Tx.h>
#include <boost/bimap/bimap.hpp>
#include <boost/optional.hpp>
#include <set>

namespace ripple {
namespace test {
namespace csf {


class Ledger
{
    friend class LedgerOracle;

public:
    struct SeqTag;
    using Seq = tagged_integer<std::uint32_t, SeqTag>;

    struct IdTag;
    using ID = tagged_integer<std::uint32_t, IdTag>;

    struct MakeGenesis {};
private:
    struct Instance
    {
        Instance() {}

        Seq seq{0};

        TxSetType txs;

        NetClock::duration closeTimeResolution = ledgerDefaultTimeResolution;

        NetClock::time_point closeTime;

        bool closeTimeAgree = true;

        ID parentID{0};

        NetClock::time_point parentCloseTime;

        std::vector<Ledger::ID> ancestors;

        auto
        asTie() const
        {
            return std::tie(seq, txs, closeTimeResolution, closeTime,
                closeTimeAgree, parentID, parentCloseTime);
        }

        friend bool
        operator==(Instance const& a, Instance const& b)
        {
            return a.asTie() == b.asTie();
        }

        friend bool
        operator!=(Instance const& a, Instance const& b)
        {
            return a.asTie() != b.asTie();
        }

        friend bool
        operator<(Instance const & a, Instance const & b)
        {
            return a.asTie() < b.asTie();
        }

        template <class Hasher>
        friend void
        hash_append(Hasher& h, Ledger::Instance const& instance)
        {
            using beast::hash_append;
            hash_append(h, instance.asTie());
        }
    };


    static const Instance genesis;

    Ledger(ID id, Instance const* i) : id_{id}, instance_{i}
    {
    }

public:
    Ledger(MakeGenesis) : instance_(&genesis)
    {
    }

    Ledger() : Ledger(MakeGenesis{})
    {
    }

    ID
    id() const
    {
        return id_;
    }

    Seq
    seq() const
    {
        return instance_->seq;
    }

    NetClock::duration
    closeTimeResolution() const
    {
        return instance_->closeTimeResolution;
    }

    bool
    closeAgree() const
    {
        return instance_->closeTimeAgree;
    }

    NetClock::time_point
    closeTime() const
    {
        return instance_->closeTime;
    }

    NetClock::time_point
    parentCloseTime() const
    {
        return instance_->parentCloseTime;
    }

    ID
    parentID() const
    {
        return instance_->parentID;
    }

    TxSetType const&
    txs() const
    {
        return instance_->txs;
    }

    
    bool
    isAncestor(Ledger const& ancestor) const;

    
    ID
    operator[](Seq seq) const;

    
    friend
    Ledger::Seq
    mismatch(Ledger const & a, Ledger const & o);

    Json::Value getJson() const;

    friend bool
    operator<(Ledger const & a, Ledger const & b)
    {
        return a.id() < b.id();
    }

private:
    ID id_{0};
    Instance const* instance_;
};



class LedgerOracle
{
    using InstanceMap = boost::bimaps::bimap<Ledger::Instance, Ledger::ID>;
    using InstanceEntry = InstanceMap::value_type;

    InstanceMap instances_;

    Ledger::ID
    nextID() const;

public:

    LedgerOracle();

    
    boost::optional<Ledger>
    lookup(Ledger::ID const & id) const;

    
    Ledger
    accept(Ledger const & curr, TxSetType const& txs,
        NetClock::duration closeTimeResolution,
        NetClock::time_point const& consensusCloseTime);

    Ledger
    accept(Ledger const& curr, Tx tx)
    {
        using namespace std::chrono_literals;
        return accept(
            curr,
            TxSetType{tx},
            curr.closeTimeResolution(),
            curr.closeTime() + 1s);
    }

    
    std::size_t
    branches(std::set<Ledger> const & ledgers) const;

};


struct LedgerHistoryHelper
{
    LedgerOracle oracle;
    Tx::ID nextTx{0};
    std::unordered_map<std::string, Ledger> ledgers;
    std::set<char> seen;

    LedgerHistoryHelper()
    {
        ledgers[""] = Ledger{Ledger::MakeGenesis{}};
    }

    
    Ledger const& operator[](std::string const& s)
    {
        auto it = ledgers.find(s);
        if (it != ledgers.end())
            return it->second;

        assert(seen.emplace(s.back()).second);

        Ledger const& parent = (*this)[s.substr(0, s.size() - 1)];
        return ledgers.emplace(s, oracle.accept(parent, ++nextTx))
            .first->second;
    }
};

}  
}  
}  

#endif








