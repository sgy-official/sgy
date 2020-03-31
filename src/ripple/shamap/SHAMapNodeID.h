

#ifndef RIPPLE_SHAMAP_SHAMAPNODEID_H_INCLUDED
#define RIPPLE_SHAMAP_SHAMAPNODEID_H_INCLUDED

#include <ripple/protocol/Serializer.h>
#include <ripple/basics/base_uint.h>
#include <ripple/beast/utility/Journal.h>
#include <ostream>
#include <string>
#include <tuple>

namespace ripple {

class SHAMapNodeID
{
private:
    uint256 mNodeID;
    int mDepth;

public:
    SHAMapNodeID ();
    SHAMapNodeID (int depth, uint256 const& hash);
    SHAMapNodeID (void const* ptr, int len);

    bool isValid () const;
    bool isRoot () const;

    void addIDRaw (Serializer& s) const;
    std::string getRawString () const;

    bool operator== (const SHAMapNodeID& n) const;
    bool operator!= (const SHAMapNodeID& n) const;

    bool operator< (const SHAMapNodeID& n) const;
    bool operator> (const SHAMapNodeID& n) const;
    bool operator<= (const SHAMapNodeID& n) const;
    bool operator>= (const SHAMapNodeID& n) const;

    std::string getString () const;
    void dump (beast::Journal journal) const;


    uint256 const& getNodeID ()  const;
    SHAMapNodeID getChildNodeID (int m) const;
    int selectBranch (uint256 const& hash) const;
    int getDepth () const;
    bool has_common_prefix(SHAMapNodeID const& other) const;

private:
    static uint256 const& Masks (int depth);

    friend std::ostream& operator<< (std::ostream& out, SHAMapNodeID const& node);

private:  
    SHAMapNodeID getParentNodeID () const;
};


inline
SHAMapNodeID::SHAMapNodeID ()
    : mDepth (0)
{
}

inline
int
SHAMapNodeID::getDepth () const
{
    return mDepth;
}

inline
uint256 const&
SHAMapNodeID::getNodeID ()  const
{
    return mNodeID;
}

inline
bool
SHAMapNodeID::isValid () const
{
    return (mDepth >= 0) && (mDepth <= 64);
}

inline
bool
SHAMapNodeID::isRoot () const
{
    return mDepth == 0;
}

inline
SHAMapNodeID
SHAMapNodeID::getParentNodeID () const
{
    assert (mDepth);
    return SHAMapNodeID (mDepth - 1,
        mNodeID & Masks (mDepth - 1));
}

inline
bool
SHAMapNodeID::operator< (const SHAMapNodeID& n) const
{
    return std::tie(mDepth, mNodeID) < std::tie(n.mDepth, n.mNodeID);
}

inline
bool
SHAMapNodeID::operator> (const SHAMapNodeID& n) const
{
    return n < *this;
}

inline
bool
SHAMapNodeID::operator<= (const SHAMapNodeID& n) const
{
    return !(n < *this);
}

inline
bool
SHAMapNodeID::operator>= (const SHAMapNodeID& n) const
{
    return !(*this < n);
}

inline
bool
SHAMapNodeID::operator== (const SHAMapNodeID& n) const
{
    return (mDepth == n.mDepth) && (mNodeID == n.mNodeID);
}

inline
bool
SHAMapNodeID::operator!= (const SHAMapNodeID& n) const
{
    return !(*this == n);
}

inline std::ostream& operator<< (std::ostream& out, SHAMapNodeID const& node)
{
    return out << node.getString ();
}

} 

#endif








