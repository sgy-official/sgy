

#ifndef RIPPLE_PROTOCOL_STVALIDATION_H_INCLUDED
#define RIPPLE_PROTOCOL_STVALIDATION_H_INCLUDED

#include <ripple/basics/Log.h>
#include <ripple/protocol/PublicKey.h>
#include <ripple/protocol/STObject.h>
#include <ripple/protocol/SecretKey.h>
#include <cstdint>
#include <functional>
#include <memory>

namespace ripple {

const std::uint32_t vfFullyCanonicalSig =
    0x80000000;  

class STValidation final : public STObject, public CountedObject<STValidation>
{
public:
    static char const*
    getCountedObjectName()
    {
        return "STValidation";
    }

    using pointer = std::shared_ptr<STValidation>;
    using ref = const std::shared_ptr<STValidation>&;

    enum { kFullFlag = 0x1 };

    
    template <class LookupNodeID>
    STValidation(
        SerialIter& sit,
        LookupNodeID&& lookupNodeID,
        bool checkSignature)
        : STObject(getFormat(), sit, sfValidation)
    {
        auto const spk = getFieldVL(sfSigningPubKey);

        if (publicKeyType(makeSlice(spk)) != KeyType::secp256k1)
        {
            JLOG (debugLog().error())
                << "Invalid public key in validation"
                << getJson (JsonOptions::none);
            Throw<std::runtime_error> ("Invalid public key in validation");
        }

        if  (checkSignature && !isValid ())
        {
            JLOG (debugLog().error())
                << "Invalid signature in validation"
                << getJson (JsonOptions::none);
            Throw<std::runtime_error> ("Invalid signature in validation");
        }

        mNodeID = lookupNodeID(PublicKey(makeSlice(spk)));
        assert(mNodeID.isNonZero());
    }

    
    struct FeeSettings
    {
        boost::optional<std::uint32_t> loadFee;
        boost::optional<std::uint64_t> baseFee;
        boost::optional<std::uint32_t> reserveBase;
        boost::optional<std::uint32_t> reserveIncrement;
    };

    
    STValidation(
        uint256 const& ledgerHash,
        std::uint32_t ledgerSeq,
        uint256 const& consensusHash,
        NetClock::time_point signTime,
        PublicKey const& publicKey,
        SecretKey const& secretKey,
        NodeID const& nodeID,
        bool isFull,
        FeeSettings const& fees,
        std::vector<uint256> const& amendments);

    STBase*
    copy(std::size_t n, void* buf) const override
    {
        return emplace(n, buf, *this);
    }

    STBase*
    move(std::size_t n, void* buf) override
    {
        return emplace(n, buf, std::move(*this));
    }

    uint256
    getLedgerHash() const;

    uint256
    getConsensusHash() const;

    NetClock::time_point
    getSignTime() const;

    NetClock::time_point
    getSeenTime() const;

    PublicKey
    getSignerPublic() const;

    NodeID
    getNodeID() const
    {
        return mNodeID;
    }

    bool
    isValid() const;

    bool
    isFull() const;

    bool
    isTrusted() const
    {
        return mTrusted;
    }

    uint256
    getSigningHash() const;

    void
    setTrusted()
    {
        mTrusted = true;
    }

    void
    setUntrusted()
    {
        mTrusted = false;
    }

    void
    setSeen(NetClock::time_point s)
    {
        mSeen = s;
    }

    Blob
    getSerialized() const;

    Blob
    getSignature() const;

private:

    static SOTemplate const&
    getFormat();

    NodeID mNodeID;
    bool mTrusted = false;
    NetClock::time_point mSeen = {};
};

} 

#endif








