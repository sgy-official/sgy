

#ifndef RIPPLE_PEERFINDER_MANAGER_H_INCLUDED
#define RIPPLE_PEERFINDER_MANAGER_H_INCLUDED

#include <ripple/peerfinder/Slot.h>
#include <ripple/beast/clock/abstract_clock.h>
#include <ripple/core/Stoppable.h>
#include <ripple/beast/utility/PropertyStream.h>
#include <boost/asio/ip/tcp.hpp>

namespace ripple {
namespace PeerFinder {

using clock_type = beast::abstract_clock <std::chrono::steady_clock>;


using IPAddresses = std::vector <beast::IP::Endpoint>;



struct Config
{
    
    int maxPeers;

    
    double outPeers;

    
    bool peerPrivate = true;

    
    bool wantIncoming;

    
    bool autoConnect;

    
    std::uint16_t listeningPort;

    
    std::string features;

    
    int ipLimit;


    
    Config ();

    
    double calcOutPeers () const;

    
    void applyTuning ();

    
    void onWrite (beast::PropertyStream::Map& map);
};



struct Endpoint
{
    Endpoint ();

    Endpoint (beast::IP::Endpoint const& ep, int hops_);

    int hops;
    beast::IP::Endpoint address;
};

bool operator< (Endpoint const& lhs, Endpoint const& rhs);


using Endpoints = std::vector <Endpoint>;



enum class Result
{
    duplicate,
    full,
    success
};


class Manager
    : public Stoppable
    , public beast::PropertyStream::Source
{
protected:
    explicit Manager (Stoppable& parent);

public:
    
    virtual ~Manager() = default;

    
    virtual void setConfig (Config const& config) = 0;

    
    virtual
    Config
    config() = 0;

    
    virtual void addFixedPeer (std::string const& name,
        std::vector <beast::IP::Endpoint> const& addresses) = 0;

    
    virtual void addFallbackStrings (std::string const& name,
        std::vector <std::string> const& strings) = 0;

    
    


    
    virtual Slot::ptr new_inbound_slot (
        beast::IP::Endpoint const& local_endpoint,
            beast::IP::Endpoint const& remote_endpoint) = 0;

    
    virtual Slot::ptr new_outbound_slot (
        beast::IP::Endpoint const& remote_endpoint) = 0;

    
    virtual void on_endpoints (Slot::ptr const& slot,
        Endpoints const& endpoints) = 0;

    
    virtual void on_closed (Slot::ptr const& slot) = 0;

    
    virtual void on_failure (Slot::ptr const& slot) = 0;

    
    virtual
    void
    onRedirects (boost::asio::ip::tcp::endpoint const& remote_address,
        std::vector<boost::asio::ip::tcp::endpoint> const& eps) = 0;


    
    virtual
    bool
    onConnected (Slot::ptr const& slot,
        beast::IP::Endpoint const& local_endpoint) = 0;

    
    virtual
    Result
    activate (Slot::ptr const& slot,
        PublicKey const& key, bool cluster) = 0;

    
    virtual
    std::vector <Endpoint>
    redirect (Slot::ptr const& slot) = 0;

    
    virtual
    std::vector <beast::IP::Endpoint>
    autoconnect() = 0;

    virtual
    std::vector<std::pair<Slot::ptr, std::vector<Endpoint>>>
    buildEndpointsForPeers() = 0;

    
    virtual
    void
    once_per_second() = 0;
};

}
}

#endif








