
#ifndef RIPPLE_TEST_CSF_TX_H_INCLUDED
#define RIPPLE_TEST_CSF_TX_H_INCLUDED
#include <ripple/beast/hash/uhash.h>
#include <ripple/beast/hash/hash_append.h>
#include <boost/function_output_iterator.hpp>
#include <boost/container/flat_set.hpp>
#include <map>
#include <ostream>
#include <string>

namespace ripple {
namespace test {
namespace csf {

class Tx
{
public:
    using ID = std::uint32_t;

    Tx(ID i) : id_{i}
    {
    }

    ID
    id() const
    {
        return id_;
    }

    bool
    operator<(Tx const& o) const
    {
        return id_ < o.id_;
    }

    bool
    operator==(Tx const& o) const
    {
        return id_ == o.id_;
    }

private:
    ID id_;
};

using TxSetType = boost::container::flat_set<Tx>;

class TxSet
{
public:
    using ID = beast::uhash<>::result_type;
    using Tx = csf::Tx;

    static ID calcID(TxSetType const & txs)
    {
        return beast::uhash<>{}(txs);
    }

    class MutableTxSet
    {
        friend class TxSet;

        TxSetType txs_;

    public:
        MutableTxSet(TxSet const& s) : txs_{s.txs_}
        {
        }

        bool
        insert(Tx const& t)
        {
            return txs_.insert(t).second;
        }

        bool
        erase(Tx::ID const& txId)
        {
            return txs_.erase(Tx{txId}) > 0;
        }
    };

    TxSet() = default;
    TxSet(TxSetType const& s) : txs_{s}, id_{calcID(txs_)}
    {
    }

    TxSet(MutableTxSet && m)
        : txs_{std::move(m.txs_)}, id_{calcID(txs_)}
    {
    }

    bool
    exists(Tx::ID const txId) const
    {
        auto it = txs_.find(Tx{txId});
        return it != txs_.end();
    }

    Tx const*
    find(Tx::ID const& txId) const
    {
        auto it = txs_.find(Tx{txId});
        if (it != txs_.end())
            return &(*it);
        return nullptr;
    }

    TxSetType const &
    txs() const
    {
        return txs_;
    }

    ID
    id() const
    {
        return id_;
    }

    
    std::map<Tx::ID, bool>
    compare(TxSet const& other) const
    {
        std::map<Tx::ID, bool> res;

        auto populate_diffs = [&res](auto const& a, auto const& b, bool s) {
            auto populator = [&](auto const& tx) { res[tx.id()] = s; };
            std::set_difference(
                a.begin(),
                a.end(),
                b.begin(),
                b.end(),
                boost::make_function_output_iterator(std::ref(populator)));
        };

        populate_diffs(txs_, other.txs_, true);
        populate_diffs(other.txs_, txs_, false);
        return res;
    }

private:
    TxSetType txs_;

    ID id_;
};


inline std::ostream&
operator<<(std::ostream& o, const Tx& t)
{
    return o << t.id();
}

template <class T>
inline std::ostream&
operator<<(std::ostream& o, boost::container::flat_set<T> const& ts)
{
    o << "{ ";
    bool do_comma = false;
    for (auto const& t : ts)
    {
        if (do_comma)
            o << ", ";
        else
            do_comma = true;
        o << t;
    }
    o << " }";
    return o;
}

inline std::string
to_string(TxSetType const& txs)
{
    std::stringstream ss;
    ss << txs;
    return ss.str();
}

template <class Hasher>
inline void
hash_append(Hasher& h, Tx const& tx)
{
    using beast::hash_append;
    hash_append(h, tx.id());
}

}  
}  
}  

#endif








