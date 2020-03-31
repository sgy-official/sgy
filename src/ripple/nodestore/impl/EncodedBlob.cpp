

#include <ripple/nodestore/impl/EncodedBlob.h>
#include <cstring>

namespace ripple {
namespace NodeStore {

void
EncodedBlob::prepare (
    std::shared_ptr<NodeObject> const& object)
{
    m_key = object->getHash().begin ();

    auto ret = m_data.alloc(object->getData ().size () + 9);

    memset (ret, 0, 8);

    ret[8] = static_cast<std::uint8_t> (object->getType ());

    memcpy (ret + 9,
        object->getData ().data(),
        object->getData ().size());
}

}
}
























