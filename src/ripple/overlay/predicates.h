

#ifndef RIPPLE_OVERLAY_PREDICATES_H_INCLUDED
#define RIPPLE_OVERLAY_PREDICATES_H_INCLUDED

#include <ripple/overlay/Message.h>
#include <ripple/overlay/Peer.h>

#include <set>

namespace ripple {


struct send_always
{
    using return_type = void;

    Message::pointer const& msg;

    send_always(Message::pointer const& m)
        : msg(m)
    { }

    void operator()(std::shared_ptr<Peer> const& peer) const
    {
        peer->send (msg);
    }
};



template <typename Predicate>
struct send_if_pred
{
    using return_type = void;

    Message::pointer const& msg;
    Predicate const& predicate;

    send_if_pred(Message::pointer const& m, Predicate const& p)
    : msg(m), predicate(p)
    { }

    void operator()(std::shared_ptr<Peer> const& peer) const
    {
        if (predicate (peer))
            peer->send (msg);
    }
};


template <typename Predicate>
send_if_pred<Predicate> send_if (
    Message::pointer const& m,
        Predicate const &f)
{
    return send_if_pred<Predicate>(m, f);
}



template <typename Predicate>
struct send_if_not_pred
{
    using return_type = void;

    Message::pointer const& msg;
    Predicate const& predicate;

    send_if_not_pred(Message::pointer const& m, Predicate const& p)
        : msg(m), predicate(p)
    { }

    void operator()(std::shared_ptr<Peer> const& peer) const
    {
        if (!predicate (peer))
            peer->send (msg);
    }
};


template <typename Predicate>
send_if_not_pred<Predicate> send_if_not (
    Message::pointer const& m,
        Predicate const &f)
{
    return send_if_not_pred<Predicate>(m, f);
}



struct match_peer
{
    Peer const* matchPeer;

    match_peer (Peer const* match = nullptr)
        : matchPeer (match)
    { }

    bool operator() (std::shared_ptr<Peer> const& peer) const
    {
        if(matchPeer && (peer.get () == matchPeer))
            return true;

        return false;
    }
};



struct peer_in_cluster
{
    match_peer skipPeer;

    peer_in_cluster (Peer const* skip = nullptr)
        : skipPeer (skip)
    { }

    bool operator() (std::shared_ptr<Peer> const& peer) const
    {
        if (skipPeer (peer))
            return false;

        if (! peer->cluster())
            return false;

        return true;
    }
};



struct peer_in_set
{
    std::set <Peer::id_t> const& peerSet;

    peer_in_set (std::set<Peer::id_t> const& peers)
        : peerSet (peers)
    { }

    bool operator() (std::shared_ptr<Peer> const& peer) const
    {
        if (peerSet.count (peer->id ()) == 0)
            return false;

        return true;
    }
};

}

#endif








