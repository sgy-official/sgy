

#ifndef RIPPLE_OVERLAY_PEER_H_INCLUDED
#define RIPPLE_OVERLAY_PEER_H_INCLUDED

#include <ripple/overlay/Message.h>
#include <ripple/basics/base_uint.h>
#include <ripple/json/json_value.h>
#include <ripple/protocol/PublicKey.h>
#include <ripple/beast/net/IPEndpoint.h>

namespace ripple {

namespace Resource {
class Charge;
}

static constexpr std::uint32_t csHopLimit = 3;


class Peer
{
public:
    using ptr = std::shared_ptr<Peer>;

    
    using id_t = std::uint32_t;

    virtual ~Peer() = default;


    virtual
    void
    send (Message::pointer const& m) = 0;

    virtual
    beast::IP::Endpoint
    getRemoteAddress() const = 0;

    
    virtual
    void
    charge (Resource::Charge const& fee) = 0;


    virtual
    id_t
    id() const = 0;

    
    virtual
    bool
    cluster() const = 0;

    virtual
    bool
    isHighLatency() const = 0;

    virtual
    int
    getScore (bool) const = 0;

    virtual
    PublicKey const&
    getNodePublic() const = 0;

    virtual
    Json::Value json() = 0;


    virtual uint256 const& getClosedLedgerHash () const = 0;
    virtual bool hasLedger (uint256 const& hash, std::uint32_t seq) const = 0;
    virtual void ledgerRange (std::uint32_t& minSeq, std::uint32_t& maxSeq) const = 0;
    virtual bool hasShard (std::uint32_t shardIndex) const = 0;
    virtual bool hasTxSet (uint256 const& hash) const = 0;
    virtual void cycleStatus () = 0;
    virtual bool supportsVersion (int version) = 0;
    virtual bool hasRange (std::uint32_t uMin, std::uint32_t uMax) = 0;
};

}

#endif








