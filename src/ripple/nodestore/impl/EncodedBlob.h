

#ifndef RIPPLE_NODESTORE_ENCODEDBLOB_H_INCLUDED
#define RIPPLE_NODESTORE_ENCODEDBLOB_H_INCLUDED

#include <ripple/basics/Buffer.h>
#include <ripple/nodestore/NodeObject.h>
#include <cstddef>

namespace ripple {
namespace NodeStore {


struct EncodedBlob
{
public:
    void prepare (std::shared_ptr<NodeObject> const& object);

    void const* getKey () const noexcept
    {
        return m_key;
    }

    std::size_t getSize () const noexcept
    {
        return m_data.size();
    }

    void const* getData () const noexcept
    {
        return reinterpret_cast<void const*>(m_data.data());
    }

private:
    void const* m_key;
    Buffer m_data;
};

}
}

#endif








