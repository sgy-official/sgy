

#ifndef RIPPLE_PEERFINDER_SLOTIMP_H_INCLUDED
#define RIPPLE_PEERFINDER_SLOTIMP_H_INCLUDED

#include <ripple/peerfinder/Slot.h>
#include <ripple/peerfinder/PeerfinderManager.h>
#include <ripple/beast/container/aged_unordered_map.h>
#include <ripple/beast/container/aged_container_utility.h>
#include <boost/optional.hpp>
#include <atomic>

namespace ripple {
namespace PeerFinder {

class SlotImp : public Slot
{
private:
    using recent_type = beast::aged_unordered_map <beast::IP::Endpoint, int>;

public:
    using ptr = std::shared_ptr <SlotImp>;

    SlotImp (beast::IP::Endpoint const& local_endpoint,
        beast::IP::Endpoint const& remote_endpoint, bool fixed,
            clock_type& clock);

    SlotImp (beast::IP::Endpoint const& remote_endpoint,
        bool fixed, clock_type& clock);

    bool inbound () const override
    {
        return m_inbound;
    }

    bool fixed () const override
    {
        return m_fixed;
    }

    bool cluster () const override
    {
        return m_cluster;
    }

    State state () const override
    {
        return m_state;
    }

    beast::IP::Endpoint const& remote_endpoint () const override
    {
        return m_remote_endpoint;
    }

    boost::optional <beast::IP::Endpoint> const& local_endpoint () const override
    {
        return m_local_endpoint;
    }

    boost::optional <PublicKey> const& public_key () const override
    {
        return m_public_key;
    }

    boost::optional<std::uint16_t> listening_port () const override
    {
        std::uint32_t const value = m_listening_port;
        if (value == unknownPort)
            return boost::none;
        return value;
    }

    void set_listening_port (std::uint16_t port)
    {
        m_listening_port = port;
    }

    void local_endpoint (beast::IP::Endpoint const& endpoint)
    {
        m_local_endpoint = endpoint;
    }

    void remote_endpoint (beast::IP::Endpoint const& endpoint)
    {
        m_remote_endpoint = endpoint;
    }

    void public_key (PublicKey const& key)
    {
        m_public_key = key;
    }

    void cluster (bool cluster_)
    {
        m_cluster = cluster_;
    }


    void state (State state_);

    void activate (clock_type::time_point const& now);

    class recent_t
    {
    public:
        explicit recent_t (clock_type& clock);

        
        void insert (beast::IP::Endpoint const& ep, int hops);

        
        bool filter (beast::IP::Endpoint const& ep, int hops);

    private:
        void expire ();

        friend class SlotImp;
        recent_type cache;
    } recent;

    void expire()
    {
        recent.expire();
    }

private:
    bool const m_inbound;
    bool const m_fixed;
    bool m_cluster;
    State m_state;
    beast::IP::Endpoint m_remote_endpoint;
    boost::optional <beast::IP::Endpoint> m_local_endpoint;
    boost::optional <PublicKey> m_public_key;

    static std::int32_t constexpr unknownPort = -1;
    std::atomic <std::int32_t> m_listening_port;

public:

    bool checked;

    bool canAccept;

    bool connectivityCheckInProgress;

    clock_type::time_point whenAcceptEndpoints;
};

}
}

#endif








