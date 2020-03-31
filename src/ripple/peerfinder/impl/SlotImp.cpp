

#include <ripple/peerfinder/impl/SlotImp.h>
#include <ripple/peerfinder/PeerfinderManager.h>
#include <ripple/peerfinder/impl/Tuning.h>

namespace ripple {
namespace PeerFinder {

SlotImp::SlotImp (beast::IP::Endpoint const& local_endpoint,
    beast::IP::Endpoint const& remote_endpoint, bool fixed,
        clock_type& clock)
    : recent (clock)
    , m_inbound (true)
    , m_fixed (fixed)
    , m_cluster (false)
    , m_state (accept)
    , m_remote_endpoint (remote_endpoint)
    , m_local_endpoint (local_endpoint)
    , m_listening_port (unknownPort)
    , checked (false)
    , canAccept (false)
    , connectivityCheckInProgress (false)
{
}

SlotImp::SlotImp (beast::IP::Endpoint const& remote_endpoint,
    bool fixed, clock_type& clock)
    : recent (clock)
    , m_inbound (false)
    , m_fixed (fixed)
    , m_cluster (false)
    , m_state (connect)
    , m_remote_endpoint (remote_endpoint)
    , m_listening_port (unknownPort)
    , checked (true)
    , canAccept (true)
    , connectivityCheckInProgress (false)
{
}

void
SlotImp::state (State state_)
{
    assert (state_ != active);

    assert (state_ != m_state);

    assert (state_ != accept && state_ != connect);

    assert (state_ != connected || (! m_inbound && m_state == connect));

    assert (state_ != closing || m_state != connect);

    m_state = state_;
}

void
SlotImp::activate (clock_type::time_point const& now)
{
    assert (m_state == accept || m_state == connected);

    m_state = active;
    whenAcceptEndpoints = now;
}


Slot::~Slot() = default;


SlotImp::recent_t::recent_t (clock_type& clock)
    : cache (clock)
{
}

void
SlotImp::recent_t::insert (beast::IP::Endpoint const& ep, int hops)
{
    auto const result (cache.emplace (ep, hops));
    if (! result.second)
    {
        if (hops <= result.first->second)
        {
            result.first->second = hops;
            cache.touch (result.first);
        }
    }
}

bool
SlotImp::recent_t::filter (beast::IP::Endpoint const& ep, int hops)
{
    auto const iter (cache.find (ep));
    if (iter == cache.end())
        return false;
    return iter->second <= hops;
}

void
SlotImp::recent_t::expire ()
{
    beast::expire (cache,
        Tuning::liveCacheSecondsToLive);
}

}
}
























