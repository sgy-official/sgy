

#ifndef RIPPLE_APP_PATHS_NODE_H_INCLUDED
#define RIPPLE_APP_PATHS_NODE_H_INCLUDED

#include <ripple/app/paths/NodeDirectory.h>
#include <ripple/app/paths/Types.h>
#include <ripple/protocol/Rate.h>
#include <ripple/protocol/UintTypes.h>
#include <boost/optional.hpp>

namespace ripple {
namespace path {

struct Node
{
    explicit Node() = default;

    using List = std::vector<Node>;

    inline bool isAccount() const
    {
        return (uFlags & STPathElement::typeAccount);
    }

    Json::Value getJson () const;

    bool operator == (Node const&) const;

    std::uint16_t uFlags;       

    AccountID account_;         

    Issue issue_;               

    boost::optional<Rate> transferRate_;         

    STAmount saRevRedeem;        
    STAmount saRevIssue;         
    STAmount saRevDeliver;       

    STAmount saFwdRedeem;        
    STAmount saFwdIssue;         
    STAmount saFwdDeliver;       

    boost::optional<Rate> rateMax;


    NodeDirectory directory;

    STAmount saOfrRate;          

    bool bEntryAdvance;          
    unsigned int uEntry;
    uint256 offerIndex_;
    SLE::pointer sleOffer;
    AccountID offerOwnerAccount_;

    bool bFundsDirty;
    STAmount saOfferFunds;
    STAmount saTakerPays;
    STAmount saTakerGets;

    
    void clear()
    {
        saRevRedeem.clear ();
        saRevIssue.clear ();
        saRevDeliver.clear ();
        saFwdDeliver.clear ();
    }
};

} 
} 

#endif








