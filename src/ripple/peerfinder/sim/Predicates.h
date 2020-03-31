

#ifndef RIPPLE_PEERFINDER_SIM_PREDICATES_H_INCLUDED
#define RIPPLE_PEERFINDER_SIM_PREDICATES_H_INCLUDED

namespace ripple {
namespace PeerFinder {
namespace Sim {



template <typename Node>
class is_remote_node_pred
{
public:
    is_remote_node_pred (Node const& node)
        : node (node)
        { }
    template <typename Link>
    bool operator() (Link const& l) const
        { return &node == &l.remote_node(); }
private:
    Node const& node;
};

template <typename Node>
is_remote_node_pred <Node> is_remote_node (Node const& node)
{
    return is_remote_node_pred <Node> (node);
}

template <typename Node>
is_remote_node_pred <Node> is_remote_node (Node const* node)
{
    return is_remote_node_pred <Node> (*node);
}




class is_remote_endpoint
{
public:
    explicit is_remote_endpoint (beast::IP::Endpoint const& address)
        : m_endpoint (address)
        { }
    template <typename Link>
    bool operator() (Link const& link) const
    {
        return link.remote_endpoint() == m_endpoint;
    }
private:
    beast::IP::Endpoint const m_endpoint;
};

}
}
}

#endif








