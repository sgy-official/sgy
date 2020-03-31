

#ifndef RIPPLE_APP_LEDGER_LEDGER_H_INCLUDED
#define RIPPLE_APP_LEDGER_LEDGER_H_INCLUDED

#include <ripple/ledger/TxMeta.h>
#include <ripple/ledger/View.h>
#include <ripple/ledger/CachedView.h>
#include <ripple/basics/CountedObject.h>
#include <ripple/core/TimeKeeper.h>
#include <ripple/protocol/Indexes.h>
#include <ripple/protocol/STLedgerEntry.h>
#include <ripple/protocol/Serializer.h>
#include <ripple/protocol/Book.h>
#include <ripple/shamap/SHAMap.h>
#include <ripple/beast/utility/Journal.h>
#include <boost/optional.hpp>
#include <mutex>

namespace ripple {

class Application;
class Job;
class TransactionMaster;

class SqliteStatement;

struct create_genesis_t
{
    explicit create_genesis_t() = default;
};
extern create_genesis_t const create_genesis;


class Ledger final
    : public std::enable_shared_from_this <Ledger>
    , public DigestAwareReadView
    , public TxsRawView
    , public CountedObject <Ledger>
{
public:
    static char const* getCountedObjectName () { return "Ledger"; }

    Ledger (Ledger const&) = delete;
    Ledger& operator= (Ledger const&) = delete;

    
    Ledger (
        create_genesis_t,
        Config const& config,
        std::vector<uint256> const& amendments,
        Family& family);

    Ledger (
        LedgerInfo const& info,
        Config const& config,
        Family& family);

    
    Ledger (
        LedgerInfo const& info,
        bool& loaded,
        bool acquire,
        Config const& config,
        Family& family,
        beast::Journal j);

    
    Ledger (Ledger const& previous,
        NetClock::time_point closeTime);

    Ledger (std::uint32_t ledgerSeq,
        NetClock::time_point closeTime, Config const& config,
            Family& family);

    ~Ledger() = default;


    bool
    open() const override
    {
        return false;
    }

    LedgerInfo const&
    info() const override
    {
        return info_;
    }

    Fees const&
    fees() const override
    {
        return fees_;
    }

    Rules const&
    rules() const override
    {
        return rules_;
    }

    bool
    exists (Keylet const& k) const override;

    boost::optional<uint256>
    succ (uint256 const& key, boost::optional<
        uint256> const& last = boost::none) const override;

    std::shared_ptr<SLE const>
    read (Keylet const& k) const override;

    std::unique_ptr<sles_type::iter_base>
    slesBegin() const override;

    std::unique_ptr<sles_type::iter_base>
    slesEnd() const override;

    std::unique_ptr<sles_type::iter_base>
    slesUpperBound(uint256 const& key) const override;

    std::unique_ptr<txs_type::iter_base>
    txsBegin() const override;

    std::unique_ptr<txs_type::iter_base>
    txsEnd() const override;

    bool
    txExists (uint256 const& key) const override;

    tx_type
    txRead (key_type const& key) const override;


    boost::optional<digest_type>
    digest (key_type const& key) const override;


    void
    rawErase (std::shared_ptr<
        SLE> const& sle) override;

    void
    rawInsert (std::shared_ptr<
        SLE> const& sle) override;

    void
    rawReplace (std::shared_ptr<
        SLE> const& sle) override;

    void
    rawDestroyXRP (XRPAmount const& fee) override
    {
        info_.drops -= fee;
    }


    void
    rawTxInsert (uint256 const& key,
        std::shared_ptr<Serializer const
            > const& txn, std::shared_ptr<
                Serializer const> const& metaData) override;


    void setValidated() const
    {
        info_.validated = true;
    }

    void setAccepted (NetClock::time_point closeTime,
        NetClock::duration closeResolution, bool correctCloseTime,
            Config const& config);

    void setImmutable (Config const& config);

    bool isImmutable () const
    {
        return mImmutable;
    }

    
    void
    setFull() const
    {
        txMap_->setFull();
        stateMap_->setFull();
        txMap_->setLedgerSeq(info_.seq);
        stateMap_->setLedgerSeq(info_.seq);
    }

    void setTotalDrops (std::uint64_t totDrops)
    {
        info_.drops = totDrops;
    }

    SHAMap const&
    stateMap() const
    {
        return *stateMap_;
    }

    SHAMap&
    stateMap()
    {
        return *stateMap_;
    }

    SHAMap const&
    txMap() const
    {
        return *txMap_;
    }

    SHAMap&
    txMap()
    {
        return *txMap_;
    }

    bool addSLE (SLE const& sle);


    void updateSkipList ();

    bool walkLedger (beast::Journal j) const;

    bool assertSane (beast::Journal ledgerJ) const;

    void make_v2();
    void invariants() const;
    void unshare() const;
private:
    class sles_iter_impl;
    class txs_iter_impl;

    bool
    setup (Config const& config);

    std::shared_ptr<SLE>
    peek (Keylet const& k) const;

    bool mImmutable;

    std::shared_ptr<SHAMap> txMap_;
    std::shared_ptr<SHAMap> stateMap_;

    std::mutex mutable mutex_;

    Fees fees_;
    Rules rules_;
    LedgerInfo info_;
};


using CachedLedger = CachedView<Ledger>;


extern
bool
pendSaveValidated(
    Application& app,
    std::shared_ptr<Ledger const> const& ledger,
    bool isSynchronous,
    bool isCurrent);

extern
std::shared_ptr<Ledger>
loadByIndex (std::uint32_t ledgerIndex,
    Application& app, bool acquire = true);

extern
std::tuple<std::shared_ptr<Ledger>, std::uint32_t, uint256>
loadLedgerHelper(std::string const& sqlSuffix,
    Application& app, bool acquire = true);

extern
std::shared_ptr<Ledger>
loadByHash (uint256 const& ledgerHash,
    Application& app, bool acquire = true);

extern
uint256
getHashByIndex(std::uint32_t index, Application& app);

extern
bool
getHashesByIndex(std::uint32_t index,
    uint256 &ledgerHash, uint256& parentHash,
        Application& app);

extern
std::map< std::uint32_t, std::pair<uint256, uint256>>
getHashesByIndex (std::uint32_t minSeq, std::uint32_t maxSeq,
    Application& app);


std::shared_ptr<STTx const>
deserializeTx (SHAMapItem const& item);


std::pair<std::shared_ptr<
    STTx const>, std::shared_ptr<
        STObject const>>
deserializeTxPlusMeta (SHAMapItem const& item);

} 

#endif








