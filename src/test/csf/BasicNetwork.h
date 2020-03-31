

#ifndef RIPPLE_TEST_CSF_BASICNETWORK_H_INCLUDED
#define RIPPLE_TEST_CSF_BASICNETWORK_H_INCLUDED

#include <test/csf/Scheduler.h>
#include <test/csf/Digraph.h>

namespace ripple {
namespace test {
namespace csf {

template <class Peer>
class BasicNetwork
{
    using peer_type = Peer;

    using clock_type = Scheduler::clock_type;

    using duration = typename clock_type::duration;

    using time_point = typename clock_type::time_point;

    struct link_type
    {
        bool inbound = false;
        duration delay{};
        time_point established{};
        link_type() = default;
        link_type(bool inbound_, duration delay_, time_point established_)
            : inbound(inbound_), delay(delay_), established(established_)
        {
        }
    };

    Scheduler& scheduler;
    Digraph<Peer, link_type> links_;

public:
    BasicNetwork(BasicNetwork const&) = delete;
    BasicNetwork&
    operator=(BasicNetwork const&) = delete;

    BasicNetwork(Scheduler& s);

    
    bool
    connect(
        Peer const& from,
        Peer const& to,
        duration const& delay = std::chrono::seconds{0});

    
    bool
    disconnect(Peer const& peer1, Peer const& peer2);

    
    template <class Function>
    void
    send(Peer const& from, Peer const& to, Function&& f);


    
    auto
    links(Peer const& from)
    {
        return links_.outEdges(from);
    }

    
    Digraph<Peer, link_type> const &
    graph() const
    {
        return links_;
    }
};
template <class Peer>
BasicNetwork<Peer>::BasicNetwork(Scheduler& s) : scheduler(s)
{
}

template <class Peer>
inline bool
BasicNetwork<Peer>::connect(
    Peer const& from,
    Peer const& to,
    duration const& delay)
{
    if (to == from)
        return false;
    time_point const now = scheduler.now();
    if(!links_.connect(from, to, link_type{false, delay, now}))
        return false;
    auto const result = links_.connect(to, from, link_type{true, delay, now});
    (void)result;
    assert(result);
    return true;
}

template <class Peer>
inline bool
BasicNetwork<Peer>::disconnect(Peer const& peer1, Peer const& peer2)
{
    if (! links_.disconnect(peer1, peer2))
        return false;
    bool r = links_.disconnect(peer2, peer1);
    (void)r;
    assert(r);
    return true;
}

template <class Peer>
template <class Function>
inline void
BasicNetwork<Peer>::send(Peer const& from, Peer const& to, Function&& f)
{
    auto link = links_.edge(from,to);
    if(!link)
        return;
    time_point const sent = scheduler.now();
    scheduler.in(
        link->delay,
        [ from, to, sent, f = std::forward<Function>(f), this ] {
            auto link = links_.edge(from, to);
            if (link && link->established <= sent)
                f();
        });
}

}  
}  
}  

#endif








