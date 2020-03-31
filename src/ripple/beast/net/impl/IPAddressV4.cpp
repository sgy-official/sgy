

#include <ripple/beast/net/IPAddressV4.h>

#include <sstream>
#include <stdexcept>

namespace beast {
namespace IP {

bool is_private (AddressV4 const& addr)
{
    return
        ((addr.to_ulong() & 0xff000000) == 0x0a000000) || 
        ((addr.to_ulong() & 0xfff00000) == 0xac100000) || 
        ((addr.to_ulong() & 0xffff0000) == 0xc0a80000) || 
        addr.is_loopback();
}

bool is_public (AddressV4 const& addr)
{
    return
        ! is_private (addr) &&
        ! addr.is_multicast();
}

char get_class (AddressV4 const& addr)
{
    static char const* table = "AAAABBCD";
    return table [(addr.to_ulong() & 0xE0000000) >> 29];
}

}
}
























