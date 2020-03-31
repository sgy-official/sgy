

#ifndef RIPPLE_PROTOCOL_PAYCHAN_H_INCLUDED
#define RIPPLE_PROTOCOL_PAYCHAN_H_INCLUDED

#include <ripple/basics/base_uint.h>
#include <ripple/protocol/HashPrefix.h>
#include <ripple/protocol/Serializer.h>
#include <ripple/protocol/XRPAmount.h>

namespace ripple {

inline
void
serializePayChanAuthorization (
    Serializer& msg,
    uint256 const& key,
    XRPAmount const& amt)
{
    msg.add32 (HashPrefix::paymentChannelClaim);
    msg.add256 (key);
    msg.add64 (amt.drops ());
}

} 

#endif








