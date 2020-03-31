

#ifndef RIPPLE_SHAMAP_SHAMAPMISSINGNODE_H_INCLUDED
#define RIPPLE_SHAMAP_SHAMAPMISSINGNODE_H_INCLUDED

#include <ripple/basics/base_uint.h>
#include <ripple/shamap/SHAMapTreeNode.h>
#include <iosfwd>
#include <stdexcept>

namespace ripple {

enum class SHAMapType
{
    TRANSACTION  = 1,    
    STATE        = 2,    
    FREE         = 3,    
};

class SHAMapMissingNode
    : public std::runtime_error
{
private:
    SHAMapType mType;
    SHAMapHash mNodeHash;
    uint256    mNodeID;
public:
    SHAMapMissingNode (SHAMapType t,
                       SHAMapHash const& nodeHash)
        : std::runtime_error ("SHAMapMissingNode")
        , mType (t)
        , mNodeHash (nodeHash)
    {
    }

    SHAMapMissingNode (SHAMapType t,
                       uint256 const& nodeID)
        : std::runtime_error ("SHAMapMissingNode")
        , mType (t)
        , mNodeID (nodeID)
    {
    }

    friend std::ostream& operator<< (std::ostream&, SHAMapMissingNode const&);
};

} 

#endif








