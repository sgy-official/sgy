

#ifndef RIPPLE_OVERLAY_OVERLAY_H_INCLUDED
#define RIPPLE_OVERLAY_OVERLAY_H_INCLUDED

#include <ripple/json/json_value.h>
#include <ripple/overlay/Peer.h>
#include <ripple/overlay/PeerSet.h>
#include <ripple/server/Handoff.h>
#include <ripple/beast/asio/ssl_bundle.h>
#include <boost/beast/http/message.hpp>
#include <ripple/core/Stoppable.h>
#include <ripple/beast/utility/PropertyStream.h>
#include <memory>
#include <type_traits>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <functional>

namespace boost { namespace asio { namespace ssl { class context; } } }

namespace ripple {


class Overlay
    : public Stoppable
    , public beast::PropertyStream::Source
{
protected:
    Overlay (Stoppable& parent)
        : Stoppable ("Overlay", parent)
        , beast::PropertyStream::Source ("peers")
    {
    }

public:
    enum class Promote
    {
        automatic,
        never,
        always
    };

    struct Setup
    {
        explicit Setup() = default;

        std::shared_ptr<boost::asio::ssl::context> context;
        bool expire = false;
        beast::IP::Address public_ip;
        int ipLimit = 0;
        std::uint32_t crawlOptions = 0;
    };

    using PeerSequence = std::vector <std::shared_ptr<Peer>>;

    virtual ~Overlay() = default;

    
    virtual
    Handoff
    onHandoff (std::unique_ptr <beast::asio::ssl_bundle>&& bundle,
        http_request_type&& request,
            boost::asio::ip::tcp::endpoint remote_address) = 0;

    
    virtual
    void
    connect (beast::IP::Endpoint const& address) = 0;

    
    virtual
    int
    limit () = 0;

    
    virtual
    std::size_t
    size () = 0;

    
    virtual
    Json::Value
    json () = 0;

    
    virtual
    PeerSequence
    getActivePeers () = 0;

    
    virtual
    void
    checkSanity (std::uint32_t index) = 0;

    
    virtual
    void
    check () = 0;

    
    virtual
    std::shared_ptr<Peer>
    findPeerByShortID (Peer::id_t const& id) = 0;

    
    virtual
    std::shared_ptr<Peer>
    findPeerByPublicKey (PublicKey const& pubKey) = 0;

    
    virtual
    void
    send (protocol::TMProposeSet& m) = 0;

    
    virtual
    void
    send (protocol::TMValidation& m) = 0;

    
    virtual
    void
    relay (protocol::TMProposeSet& m,
        uint256 const& uid) = 0;

    
    virtual
    void
    relay (protocol::TMValidation& m,
        uint256 const& uid) = 0;

    
    template <typename UnaryFunc>
    std::enable_if_t<! std::is_void<
            typename UnaryFunc::return_type>::value,
                typename UnaryFunc::return_type>
    foreach (UnaryFunc f)
    {
        for (auto const& p : getActivePeers())
            f (p);
        return f();
    }

    
    template <class Function>
    std::enable_if_t <
        std::is_void <typename Function::return_type>::value,
        typename Function::return_type
    >
    foreach(Function f)
    {
        for (auto const& p : getActivePeers())
            f (p);
    }

    
    virtual
    std::size_t
    selectPeers (PeerSet& set, std::size_t limit, std::function<
        bool(std::shared_ptr<Peer> const&)> score) = 0;

    
    virtual void incJqTransOverflow() = 0;
    virtual std::uint64_t getJqTransOverflow() const = 0;

    
    virtual void incPeerDisconnect() = 0;
    virtual std::uint64_t getPeerDisconnect() const = 0;
    virtual void incPeerDisconnectCharges() = 0;
    virtual std::uint64_t getPeerDisconnectCharges() const = 0;

    
    virtual
    Json::Value
    crawlShards(bool pubKey, std::uint32_t hops) = 0;
};

struct ScoreHasLedger
{
    uint256 const& hash_;
    std::uint32_t seq_;
    bool operator()(std::shared_ptr<Peer> const&) const;

    ScoreHasLedger (uint256 const& hash, std::uint32_t seq)
        : hash_ (hash), seq_ (seq)
    {}
};

struct ScoreHasTxSet
{
    uint256 const& hash_;
    bool operator()(std::shared_ptr<Peer> const&) const;

    ScoreHasTxSet (uint256 const& hash) : hash_ (hash)
    {}
};

}

#endif








