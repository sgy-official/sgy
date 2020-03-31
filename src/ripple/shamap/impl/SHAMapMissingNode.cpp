

#include <ripple/shamap/SHAMapMissingNode.h>
#include <ripple/beast/utility/Zero.h>
#include <ostream>

namespace ripple {

std::ostream&
operator<< (std::ostream& out, const SHAMapMissingNode& mn)
{
    switch (mn.mType)
    {
    case SHAMapType::TRANSACTION:
        out << "Missing/TXN(";
        break;

    case SHAMapType::STATE:
        out << "Missing/STA(";
        break;

    case SHAMapType::FREE:
    default:
        out << "Missing/(";
        break;
    };

    if (mn.mNodeHash == beast::zero)
        out << "id : " << mn.mNodeID;
    else
        out << "hash : " << mn.mNodeHash;
    out << ")";
    return out;
}

} 
























