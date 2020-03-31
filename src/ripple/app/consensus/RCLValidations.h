

#ifndef RIPPLE_APP_CONSENSUSS_VALIDATIONS_H_INCLUDED
#define RIPPLE_APP_CONSENSUSS_VALIDATIONS_H_INCLUDED

#include <ripple/app/ledger/Ledger.h>
#include <ripple/basics/ScopedLock.h>
#include <ripple/consensus/Validations.h>
#include <ripple/protocol/Protocol.h>
#include <ripple/protocol/RippleLedgerHash.h>
#include <ripple/protocol/STValidation.h>
#include <vector>

namespace ripple {

class Application;


class RCLValidation
{
    STValidation::pointer val_;
public:
    using NodeKey = ripple::PublicKey;
    using NodeID = ripple::NodeID;

    
    RCLValidation(STValidation::pointer const& v) : val_{v}
    {
    }

    uint256
    ledgerID() const
    {
        return val_->getLedgerHash();
    }

    std::uint32_t
    seq() const
    {
        return val_->getFieldU32(sfLedgerSequence);
    }

    NetClock::time_point
    signTime() const
    {
        return val_->getSignTime();
    }

    NetClock::time_point
    seenTime() const
    {
        return val_->getSeenTime();
    }

    PublicKey
    key() const
    {
        return val_->getSignerPublic();
    }

    NodeID
    nodeID() const
    {
        return val_->getNodeID();
    }

    bool
    trusted() const
    {
        return val_->isTrusted();
    }

    void
    setTrusted()
    {
        val_->setTrusted();
    }

    void
    setUntrusted()
    {
        val_->setUntrusted();
    }

    bool
    full() const
    {
        return val_->isFull();
    }

    boost::optional<std::uint32_t>
    loadFee() const
    {
        return ~(*val_)[~sfLoadFee];
    }

    STValidation::pointer
    unwrap() const
    {
        return val_;
    }

};


class RCLValidatedLedger
{
public:
    using ID = LedgerHash;
    using Seq = LedgerIndex;
    struct MakeGenesis
    {
        explicit MakeGenesis() = default;
    };

    RCLValidatedLedger(MakeGenesis);

    RCLValidatedLedger(
        std::shared_ptr<Ledger const> const& ledger,
        beast::Journal j);

    Seq
    seq() const;

    ID
    id() const;

    
    ID operator[](Seq const& s) const;

    friend Seq
    mismatch(RCLValidatedLedger const& a, RCLValidatedLedger const& b);

    Seq
    minSeq() const;

private:
    ID ledgerID_;
    Seq ledgerSeq_;
    std::vector<uint256> ancestors_;
    beast::Journal j_;
};


class RCLValidationsAdaptor
{
public:
    using Mutex = std::mutex;
    using Validation = RCLValidation;
    using Ledger = RCLValidatedLedger;

    RCLValidationsAdaptor(Application& app, beast::Journal j);

    
    NetClock::time_point
    now() const;

    
    void
    onStale(RCLValidation&& v);

    
    void
    flush(hash_map<NodeID, RCLValidation>&& remaining);

    
    boost::optional<RCLValidatedLedger>
    acquire(LedgerHash const & id);

    beast::Journal
    journal() const
    {
        return j_;
    }

private:
    using ScopedLockType = std::lock_guard<Mutex>;
    using ScopedUnlockType = GenericScopedUnlock<Mutex>;

    Application& app_;
    beast::Journal j_;

    std::mutex staleLock_;
    std::vector<RCLValidation> staleValidations_;
    bool staleWriting_ = false;

    void
    doStaleWrite(ScopedLockType&);
};

using RCLValidations = Validations<RCLValidationsAdaptor>;



bool
handleNewValidation(
    Application& app,
    STValidation::ref val,
    std::string const& source);

}  

#endif








