

#ifndef RIPPLE_NODESTORE_DECODEDBLOB_H_INCLUDED
#define RIPPLE_NODESTORE_DECODEDBLOB_H_INCLUDED

#include <ripple/nodestore/NodeObject.h>

namespace ripple {
namespace NodeStore {


class DecodedBlob
{
public:
    
    DecodedBlob (void const* key, void const* value, int valueBytes);

    
    bool wasOk () const noexcept { return m_success; }

    
    std::shared_ptr<NodeObject> createObject ();

private:
    bool m_success;

    void const* m_key;
    NodeObjectType m_objectType;
    unsigned char const* m_objectData;
    int m_dataBytes;
};

}
}

#endif








