

#ifndef RIPPLE_APP_CONSENSUS_RCLCXPEERPOS_H_INCLUDED
#define RIPPLE_APP_CONSENSUS_RCLCXPEERPOS_H_INCLUDED

#include <ripple/basics/CountedObject.h>
#include <ripple/basics/base_uint.h>
#include <ripple/beast/hash/hash_append.h>
#include <ripple/consensus/ConsensusProposal.h>
#include <ripple/json/json_value.h>
#include <ripple/protocol/HashPrefix.h>
#include <ripple/protocol/PublicKey.h>
#include <ripple/protocol/SecretKey.h>
#include <chrono>
#include <cstdint>
#include <string>

namespace ripple {


class RCLCxPeerPos
{
public:
    using Proposal = ConsensusProposal<NodeID, uint256, uint256>;

    

    RCLCxPeerPos(
        PublicKey const& publicKey,
        Slice const& signature,
        uint256 const& suppress,
        Proposal&& proposal);

    uint256
    signingHash() const;

    bool
    checkSign() const;

    Slice
    signature() const
    {
        return data_->signature_;
    }

    PublicKey const&
    publicKey() const
    {
        return data_->publicKey_;
    }

    uint256 const&
    suppressionID() const
    {
        return data_->suppression_;
    }

    Proposal const &
    proposal() const
    {
        return data_->proposal_;
    }

    Json::Value
    getJson() const;

private:

    struct Data : public CountedObject<Data>
    {
        PublicKey publicKey_;
        Buffer signature_;
        uint256 suppression_;
        Proposal proposal_;

        Data(
            PublicKey const& publicKey,
            Slice const& signature,
            uint256 const& suppress,
            Proposal&& proposal);

        static char const*
        getCountedObjectName()
        {
            return "RCLCxPeerPos::Data";
        }
    };

    std::shared_ptr<Data> data_;

    template <class Hasher>
    void
    hash_append(Hasher& h) const
    {
        using beast::hash_append;
        hash_append(h, HashPrefix::proposal);
        hash_append(h, std::uint32_t(proposal().proposeSeq()));
        hash_append(h, proposal().closeTime());
        hash_append(h, proposal().prevLedger());
        hash_append(h, proposal().position());
    }

};


uint256
proposalUniqueId(
    uint256 const& proposeHash,
    uint256 const& previousLedger,
    std::uint32_t proposeSeq,
    NetClock::time_point closeTime,
    Slice const& publicKey,
    Slice const& signature);

}  

#endif








