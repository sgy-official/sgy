

#include <ripple/peerfinder/PeerfinderManager.h>

namespace ripple {
namespace PeerFinder {

Endpoint::Endpoint ()
    : hops (0)
{
}

Endpoint::Endpoint (beast::IP::Endpoint const& ep, int hops_)
    : hops (hops_)
    , address (ep)
{
}

bool operator< (Endpoint const& lhs, Endpoint const& rhs)
{
    return lhs.address < rhs.address;
}

}
}
























