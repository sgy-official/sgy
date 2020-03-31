

#include <ripple/protocol/Serializer.h>
#include <ripple/shamap/SHAMapItem.h>

namespace ripple {

class SHAMap;

SHAMapItem::SHAMapItem (uint256 const& tag, Blob const& data)
    : tag_(tag)
    , data_(data)
{
}

SHAMapItem::SHAMapItem (uint256 const& tag, const Serializer& data)
    : tag_ (tag)
    , data_(data.peekData())
{
}

SHAMapItem::SHAMapItem (uint256 const& tag, Serializer&& data)
    : tag_ (tag)
    , data_(std::move(data.modData()))
{
}

} 
























