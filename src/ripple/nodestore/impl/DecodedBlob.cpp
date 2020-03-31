

#include <ripple/basics/safe_cast.h>
#include <ripple/nodestore/impl/DecodedBlob.h>
#include <algorithm>
#include <cassert>

namespace ripple {
namespace NodeStore {

DecodedBlob::DecodedBlob (void const* key, void const* value, int valueBytes)
{
    

    m_success = false;
    m_key = key;
    m_objectType = hotUNKNOWN;
    m_objectData = nullptr;
    m_dataBytes = std::max (0, valueBytes - 9);


    if (valueBytes > 8)
    {
        unsigned char const* byte = static_cast <unsigned char const*> (value);
        m_objectType = safe_cast <NodeObjectType> (byte [8]);
    }

    if (valueBytes > 9)
    {
        m_objectData = static_cast <unsigned char const*> (value) + 9;

        switch (m_objectType)
        {
        default:
            break;

        case hotUNKNOWN:
        case hotLEDGER:
        case hotACCOUNT_NODE:
        case hotTRANSACTION_NODE:
            m_success = true;
            break;
        }
    }
}

std::shared_ptr<NodeObject> DecodedBlob::createObject ()
{
    assert (m_success);

    std::shared_ptr<NodeObject> object;

    if (m_success)
    {
        Blob data(m_objectData, m_objectData + m_dataBytes);

        object = NodeObject::createObject (
            m_objectType, std::move(data), uint256::fromVoid(m_key));
    }

    return object;
}

}
}
























