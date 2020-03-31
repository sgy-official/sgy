

#ifndef BEAST_NET_IPENDPOINT_H_INCLUDED
#define BEAST_NET_IPENDPOINT_H_INCLUDED

#include <ripple/beast/net/IPAddress.h>
#include <ripple/beast/hash/hash_append.h>
#include <ripple/beast/hash/uhash.h>
#include <cstdint>
#include <ios>
#include <string>

namespace beast {
namespace IP {

using Port = std::uint16_t;


class Endpoint
{
public:
    
    Endpoint ();

    
    explicit Endpoint (Address const& addr, Port port = 0);

    
    static std::pair <Endpoint, bool> from_string_checked (std::string const& s);
    static Endpoint from_string (std::string const& s);

    
    std::string to_string () const;

    
    Port port () const
        { return m_port; }

    
    Endpoint at_port (Port port) const
        { return Endpoint (m_addr, port); }

    
    Address const& address () const
        { return m_addr; }

    
    
    bool is_v4 () const
        { return m_addr.is_v4(); }
    bool is_v6 () const
        { return m_addr.is_v6(); }
    AddressV4 const to_v4 () const
        { return m_addr.to_v4 (); }
    AddressV6 const to_v6 () const
        { return m_addr.to_v6 (); }
    

    
    
    friend bool operator== (Endpoint const& lhs, Endpoint const& rhs);
    friend bool operator<  (Endpoint const& lhs, Endpoint const& rhs);

    friend bool operator!= (Endpoint const& lhs, Endpoint const& rhs)
        { return ! (lhs == rhs); }
    friend bool operator>  (Endpoint const& lhs, Endpoint const& rhs)
        { return rhs < lhs; }
    friend bool operator<= (Endpoint const& lhs, Endpoint const& rhs)
        { return ! (lhs > rhs); }
    friend bool operator>= (Endpoint const& lhs, Endpoint const& rhs)
        { return ! (rhs > lhs); }
    

    template <class Hasher>
    friend
    void
    hash_append (Hasher& h, Endpoint const& endpoint)
    {
        using ::beast::hash_append;
        hash_append(h, endpoint.m_addr, endpoint.m_port);
    }

private:
    Address m_addr;
    Port m_port;
};




inline bool  is_loopback (Endpoint const& endpoint)
    { return is_loopback (endpoint.address ()); }


inline bool  is_unspecified (Endpoint const& endpoint)
    { return is_unspecified (endpoint.address ()); }


inline bool  is_multicast (Endpoint const& endpoint)
    { return is_multicast (endpoint.address ()); }


inline bool  is_private (Endpoint const& endpoint)
    { return is_private (endpoint.address ()); }


inline bool  is_public (Endpoint const& endpoint)
    { return is_public (endpoint.address ()); }



inline std::string to_string (Endpoint const& endpoint)
    { return endpoint.to_string(); }


template <typename OutputStream>
OutputStream& operator<< (OutputStream& os, Endpoint const& endpoint)
{
    os << to_string (endpoint);
    return os;
}


std::istream& operator>> (std::istream& is, Endpoint& endpoint);

}
}


namespace std {

template <>
struct hash <::beast::IP::Endpoint>
{
    explicit hash() = default;

    std::size_t operator() (::beast::IP::Endpoint const& endpoint) const
        { return ::beast::uhash<>{} (endpoint); }
};
}

namespace boost {

template <>
struct hash <::beast::IP::Endpoint>
{
    explicit hash() = default;

    std::size_t operator() (::beast::IP::Endpoint const& endpoint) const
        { return ::beast::uhash<>{} (endpoint); }
};
}

#endif








