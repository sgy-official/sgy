

#ifndef RIPPLE_RESOURCE_KEY_H_INCLUDED
#define RIPPLE_RESOURCE_KEY_H_INCLUDED

#include <ripple/resource/impl/Kind.h>
#include <ripple/beast/net/IPEndpoint.h>
#include <cassert>

namespace ripple {
namespace Resource {

struct Key
{
    Kind kind;
    beast::IP::Endpoint address;

    Key () = delete;

    Key (Kind k, beast::IP::Endpoint const& addr)
        : kind(k)
        , address(addr)
    {}

    struct hasher
    {
        std::size_t operator() (Key const& v) const
        {
            return m_addr_hash (v.address);
        }

    private:
        beast::uhash <> m_addr_hash;
    };

    struct key_equal
    {
        explicit key_equal() = default;

        bool operator() (Key const& lhs, Key const& rhs) const
        {
            return lhs.kind == rhs.kind && lhs.address == rhs.address;
        }

    private:
    };
};

}
}

#endif








