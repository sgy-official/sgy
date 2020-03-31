

#ifndef RIPPLE_PEERFINDER_SLOT_H_INCLUDED
#define RIPPLE_PEERFINDER_SLOT_H_INCLUDED

#include <ripple/protocol/PublicKey.h>
#include <ripple/beast/net/IPEndpoint.h>
#include <boost/optional.hpp>
#include <memory>

namespace ripple {
namespace PeerFinder {


class Slot
{
public:
    using ptr = std::shared_ptr <Slot>;

    enum State
    {
        accept,
        connect,
        connected,
        active,
        closing
    };

    virtual ~Slot () = 0;

    
    virtual bool inbound () const = 0;

    
    virtual bool fixed () const = 0;

    
    virtual bool cluster () const = 0;

    
    virtual State state () const = 0;

    
    virtual beast::IP::Endpoint const& remote_endpoint () const = 0;

    
    virtual boost::optional <beast::IP::Endpoint> const& local_endpoint () const = 0;

    virtual boost::optional<std::uint16_t> listening_port () const = 0;

    
    virtual boost::optional <PublicKey> const& public_key () const = 0;
};

}
}

#endif








