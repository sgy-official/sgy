

#ifndef RIPPLE_PROTOCOL_STLEDGERENTRY_H_INCLUDED
#define RIPPLE_PROTOCOL_STLEDGERENTRY_H_INCLUDED

#include <ripple/protocol/Indexes.h>
#include <ripple/protocol/STObject.h>

namespace ripple {

class Invariants_test;

class STLedgerEntry final
    : public STObject
    , public CountedObject <STLedgerEntry>
{
    friend Invariants_test; 

public:
    static char const* getCountedObjectName () { return "STLedgerEntry"; }

    using pointer = std::shared_ptr<STLedgerEntry>;
    using ref     = const std::shared_ptr<STLedgerEntry>&;

    
    explicit
    STLedgerEntry (Keylet const& k);

    STLedgerEntry (LedgerEntryType type,
            uint256 const& key)
        : STLedgerEntry(Keylet(type, key))
    {
    }

    STLedgerEntry (SerialIter & sit, uint256 const& index);
    STLedgerEntry(SerialIter&& sit, uint256 const& index)
        : STLedgerEntry(sit, index) {}

    STLedgerEntry (STObject const& object, uint256 const& index);

    STBase*
    copy (std::size_t n, void* buf) const override
    {
        return emplace(n, buf, *this);
    }

    STBase*
    move (std::size_t n, void* buf) override
    {
        return emplace(n, buf, std::move(*this));
    }

    SerializedTypeID getSType () const override
    {
        return STI_LEDGERENTRY;
    }

    std::string getFullText () const override;

    std::string getText () const override;

    Json::Value getJson (JsonOptions options) const override;

    
    uint256 const&
    key() const
    {
        return key_;
    }

    LedgerEntryType getType () const
    {
        return type_;
    }

    bool isThreadedType() const;

    bool thread (
        uint256 const& txID,
        std::uint32_t ledgerSeq,
        uint256 & prevTxID,
        std::uint32_t & prevLedgerID);

private:
    
    void setSLEType ();

private:
    uint256 key_;
    LedgerEntryType type_;
};

using SLE = STLedgerEntry;

} 

#endif








