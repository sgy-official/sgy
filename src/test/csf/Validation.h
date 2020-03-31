
#ifndef RIPPLE_TEST_CSF_VALIDATION_H_INCLUDED
#define RIPPLE_TEST_CSF_VALIDATION_H_INCLUDED

#include <boost/optional.hpp>
#include <memory>
#include <ripple/basics/tagged_integer.h>
#include <test/csf/ledgers.h>
#include <utility>

namespace ripple {
namespace test {
namespace csf {


struct PeerIDTag;
using PeerID = tagged_integer<std::uint32_t, PeerIDTag>;


using PeerKey =  std::pair<PeerID, std::uint32_t>;


class Validation
{
    Ledger::ID ledgerID_{0};
    Ledger::Seq seq_{0};

    NetClock::time_point signTime_;
    NetClock::time_point seenTime_;
    PeerKey key_;
    PeerID nodeID_{0};
    bool trusted_ = false;
    bool full_ = false;
    boost::optional<std::uint32_t> loadFee_;

public:
    using NodeKey = PeerKey;
    using NodeID = PeerID;

    Validation(Ledger::ID id,
        Ledger::Seq seq,
        NetClock::time_point sign,
        NetClock::time_point seen,
        PeerKey key,
        PeerID nodeID,
        bool full,
        boost::optional<std::uint32_t> loadFee = boost::none)
        : ledgerID_{id}
        , seq_{seq}
        , signTime_{sign}
        , seenTime_{seen}
        , key_{key}
        , nodeID_{nodeID}
        , full_{full}
        , loadFee_{loadFee}
    {
    }

    Ledger::ID
    ledgerID() const
    {
        return ledgerID_;
    }

    Ledger::Seq
    seq() const
    {
        return seq_;
    }

    NetClock::time_point
    signTime() const
    {
        return signTime_;
    }

    NetClock::time_point
    seenTime() const
    {
        return seenTime_;
    }

    PeerKey
    key() const
    {
        return key_;
    }

    PeerID
    nodeID() const
    {
        return nodeID_;
    }

    bool
    trusted() const
    {
        return trusted_;
    }

    bool
    full() const
    {
        return full_;
    }


    boost::optional<std::uint32_t>
    loadFee() const
    {
        return loadFee_;
    }

    Validation const&
    unwrap() const
    {
        return *this;
    }

    auto
    asTie() const
    {
        return std::tie(
            ledgerID_,
            seq_,
            signTime_,
            seenTime_,
            key_,
            nodeID_,
            loadFee_,
            full_);
    }
    bool
    operator==(Validation const& o) const
    {
        return asTie() == o.asTie();
    }

    bool
    operator<(Validation const& o) const
    {
        return asTie() < o.asTie();
    }

    void
    setTrusted()
    {
        trusted_ = true;
    }

    void
    setUntrusted()
    {
        trusted_ = false;
    }

    void
    setSeen(NetClock::time_point seen)
    {
        seenTime_ = seen;
    }
};

}  
}  
}  
#endif







