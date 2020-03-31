

#ifndef RIPPLE_LEDGER_READVIEW_H_INCLUDED
#define RIPPLE_LEDGER_READVIEW_H_INCLUDED

#include <ripple/ledger/detail/ReadViewFwdRange.h>
#include <ripple/basics/chrono.h>
#include <ripple/protocol/Indexes.h>
#include <ripple/protocol/IOUAmount.h>
#include <ripple/protocol/Protocol.h>
#include <ripple/protocol/STLedgerEntry.h>
#include <ripple/protocol/STTx.h>
#include <ripple/protocol/XRPAmount.h>
#include <ripple/beast/hash/uhash.h>
#include <ripple/beast/utility/Journal.h>
#include <boost/optional.hpp>
#include <cassert>
#include <cstdint>
#include <memory>
#include <unordered_set>

namespace ripple {


struct Fees
{
    std::uint64_t base = 0;         
    std::uint32_t units = 0;        
    std::uint32_t reserve = 0;      
    std::uint32_t increment = 0;    

    explicit Fees() = default;
    Fees (Fees const&) = default;
    Fees& operator= (Fees const&) = default;

    
    XRPAmount
    accountReserve (std::size_t ownerCount) const
    {
        return { reserve + ownerCount * increment };
    }
};



struct LedgerInfo
{
    explicit LedgerInfo() = default;


    LedgerIndex seq = 0;
    NetClock::time_point parentCloseTime = {};


    uint256 hash = beast::zero;
    uint256 txHash = beast::zero;
    uint256 accountHash = beast::zero;
    uint256 parentHash = beast::zero;

    XRPAmount drops = beast::zero;

    bool mutable validated = false;
    bool accepted = false;

    int closeFlags = 0;

    NetClock::duration closeTimeResolution = {};

    NetClock::time_point closeTime = {};
};


class DigestAwareReadView;


class Rules
{
private:
    class Impl;

    std::shared_ptr<Impl const> impl_;

public:
    Rules (Rules const&) = default;
    Rules& operator= (Rules const&) = default;

    Rules() = delete;

    
    explicit Rules(std::unordered_set<uint256, beast::uhash<>> const& presets);

    
    explicit Rules(
        DigestAwareReadView const& ledger,
            std::unordered_set<uint256, beast::uhash<>> const& presets);

    
    bool
    enabled (uint256 const& id) const;

    
    bool
    changed (DigestAwareReadView const& ledger) const;

    
    bool
    operator== (Rules const&) const;

    bool
    operator!= (Rules const& other) const
    {
        return ! (*this == other);
    }
};



class ReadView
{
public:
    using tx_type =
        std::pair<std::shared_ptr<STTx const>,
            std::shared_ptr<STObject const>>;

    using key_type = uint256;

    using mapped_type =
        std::shared_ptr<SLE const>;

    struct sles_type : detail::ReadViewFwdRange<
        std::shared_ptr<SLE const>>
    {
        explicit sles_type (ReadView const& view);
        iterator begin() const;
        iterator const& end() const;
        iterator upper_bound(key_type const& key) const;
    };

    struct txs_type
        : detail::ReadViewFwdRange<tx_type>
    {
        explicit txs_type (ReadView const& view);
        bool empty() const;
        iterator begin() const;
        iterator const& end() const;
    };

    virtual ~ReadView() = default;

    ReadView& operator= (ReadView&& other) = delete;
    ReadView& operator= (ReadView const& other) = delete;

    ReadView ()
        : sles(*this)
        , txs(*this)
    {
    }

    ReadView (ReadView const& other)
        : sles(*this)
        , txs(*this)
    {
    }

    ReadView (ReadView&& other)
        : sles(*this)
        , txs(*this)
    {
    }

    
    virtual
    LedgerInfo const&
    info() const = 0;

    
    virtual
    bool
    open() const = 0;

    
    NetClock::time_point
    parentCloseTime() const
    {
        return info().parentCloseTime;
    }

    
    LedgerIndex
    seq() const
    {
        return info().seq;
    }

    
    virtual
    Fees const&
    fees() const = 0;

    
    virtual
    Rules const&
    rules() const = 0;

    
    virtual
    bool
    exists (Keylet const& k) const = 0;

    
    virtual
    boost::optional<key_type>
    succ (key_type const& key, boost::optional<
        key_type> const& last = boost::none) const = 0;

    
    virtual
    std::shared_ptr<SLE const>
    read (Keylet const& k) const = 0;

    virtual
    STAmount
    balanceHook (AccountID const& account,
        AccountID const& issuer,
            STAmount const& amount) const
    {
        return amount;
    }

    virtual
    std::uint32_t
    ownerCountHook (AccountID const& account,
        std::uint32_t count) const
    {
        return count;
    }

    virtual
    std::unique_ptr<sles_type::iter_base>
    slesBegin() const = 0;

    virtual
    std::unique_ptr<sles_type::iter_base>
    slesEnd() const = 0;

    virtual
    std::unique_ptr<sles_type::iter_base>
    slesUpperBound(key_type const& key) const = 0;

    virtual
    std::unique_ptr<txs_type::iter_base>
    txsBegin() const = 0;

    virtual
    std::unique_ptr<txs_type::iter_base>
    txsEnd() const = 0;

    
    virtual
    bool
    txExists (key_type const& key) const = 0;

    
    virtual
    tx_type
    txRead (key_type const& key) const = 0;


    
    sles_type sles;

    txs_type txs;
};



class DigestAwareReadView
    : public ReadView
{
public:
    using digest_type = uint256;

    DigestAwareReadView () = default;
    DigestAwareReadView (const DigestAwareReadView&) = default;

    
    virtual
    boost::optional<digest_type>
    digest (key_type const& key) const = 0;
};


static
std::uint32_t const sLCF_NoConsensusTime = 0x01;

static
std::uint32_t const sLCF_SHAMapV2 = 0x02;

inline
bool getCloseAgree (LedgerInfo const& info)
{
    return (info.closeFlags & sLCF_NoConsensusTime) == 0;
}

inline
bool getSHAMapV2 (LedgerInfo const& info)
{
    return (info.closeFlags & sLCF_SHAMapV2) != 0;
}

void addRaw (LedgerInfo const&, Serializer&);

} 

#include <ripple/ledger/detail/ReadViewFwdRange.ipp>

#endif








