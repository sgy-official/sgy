

#ifndef RIPPLE_NODESTORE_NODEOBJECT_H_INCLUDED
#define RIPPLE_NODESTORE_NODEOBJECT_H_INCLUDED

#include <ripple/basics/Blob.h>
#include <ripple/basics/CountedObject.h>
#include <ripple/protocol/Protocol.h>


namespace ripple {


enum NodeObjectType
    : std::uint32_t
{
    hotUNKNOWN = 0,
    hotLEDGER = 1,
    hotACCOUNT_NODE = 3,
    hotTRANSACTION_NODE = 4
};


class NodeObject : public CountedObject <NodeObject>
{
public:
    static char const* getCountedObjectName () { return "NodeObject"; }

    enum
    {
        
        keyBytes = 32,
    };

private:
    struct PrivateAccess
    {
        explicit PrivateAccess() = default;
    };
public:
    NodeObject (NodeObjectType type,
                Blob&& data,
                uint256 const& hash,
                PrivateAccess);

    
    static
    std::shared_ptr<NodeObject>
    createObject (NodeObjectType type,
        Blob&& data, uint256 const& hash);

    
    NodeObjectType getType () const;

    
    uint256 const& getHash () const;

    
    Blob const& getData () const;

private:
    NodeObjectType mType;
    uint256 mHash;
    Blob mData;
};

}

#endif








