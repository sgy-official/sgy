

#include <ripple/app/consensus/RCLCxPeerPos.h>
#include <ripple/core/Config.h>
#include <ripple/protocol/HashPrefix.h>
#include <ripple/protocol/jss.h>
#include <ripple/protocol/Serializer.h>
#include <ripple/protocol/digest.h>

namespace ripple {

RCLCxPeerPos::RCLCxPeerPos(
    PublicKey const& publicKey,
    Slice const& signature,
    uint256 const& suppression,
    Proposal&& proposal)
    : data_{std::make_shared<Data>(
          publicKey,
          signature,
          suppression,
          std::move(proposal))}
{
}

uint256
RCLCxPeerPos::signingHash() const
{
    return sha512Half(
        HashPrefix::proposal,
        std::uint32_t(proposal().proposeSeq()),
        proposal().closeTime().time_since_epoch().count(),
        proposal().prevLedger(),
        proposal().position());
}

bool
RCLCxPeerPos::checkSign() const
{
    return verifyDigest(
        publicKey(), signingHash(), signature(), false);
}


Json::Value
RCLCxPeerPos::getJson() const
{
    auto ret = proposal().getJson();

    if (publicKey().size())
        ret[jss::peer_id] = toBase58(TokenType::NodePublic, publicKey());

    return ret;
}

uint256
proposalUniqueId(
    uint256 const& proposeHash,
    uint256 const& previousLedger,
    std::uint32_t proposeSeq,
    NetClock::time_point closeTime,
    Slice const& publicKey,
    Slice const& signature)
{
    Serializer s(512);
    s.add256(proposeHash);
    s.add256(previousLedger);
    s.add32(proposeSeq);
    s.add32(closeTime.time_since_epoch().count());
    s.addVL(publicKey);
    s.addVL(signature);

    return s.getSHA512Half();
}

RCLCxPeerPos::Data::Data(
    PublicKey const& publicKey,
    Slice const& signature,
    uint256 const& suppress,
    Proposal&& proposal)
    : publicKey_{publicKey}
    , signature_{signature}
    , suppression_{suppress}
    , proposal_{std::move(proposal)}
{
}

}  
























