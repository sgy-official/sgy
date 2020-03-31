

#include <ripple/beast/net/IPAddressV6.h>
#include <ripple/beast/net/IPAddressV4.h>

namespace beast {
namespace IP {

bool is_private (AddressV6 const& addr)
{
    return ((addr.to_bytes()[0] & 0xfd) || 
            (addr.is_v4_mapped() && is_private(addr.to_v4())) );
}

bool is_public (AddressV6 const& addr)
{
    return
        ! is_private (addr) &&
        ! addr.is_multicast();
}


}
}
























