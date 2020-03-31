

#ifndef RIPPLE_APP_PATHS_PATHSTATE_H_INCLUDED
#define RIPPLE_APP_PATHS_PATHSTATE_H_INCLUDED

#include <ripple/app/paths/Node.h>
#include <ripple/app/paths/Types.h>
#include <ripple/ledger/PaymentSandbox.h>
#include <boost/optional.hpp>

namespace ripple {

class PathState : public CountedObject <PathState>
{
  public:
    using OfferIndexList = std::vector<uint256>;
    using Ptr = std::shared_ptr<PathState>;
    using List = std::vector<Ptr>;

    PathState (PaymentSandbox const& parent,
            STAmount const& saSend,
               STAmount const& saSendMax,
                   beast::Journal j)
        : mIndex (0)
        , uQuality (0)
        , saInReq (saSendMax)
        , saOutReq (saSend)
        , j_ (j)
    {
        view_.emplace(&parent);
    }

    void reset(STAmount const& in, STAmount const& out);

    TER expandPath (
        STPath const&    spSourcePath,
        AccountID const& uReceiverID,
        AccountID const& uSenderID
    );

    path::Node::List& nodes() { return nodes_; }

    STAmount const& inPass() const { return saInPass; }
    STAmount const& outPass() const { return saOutPass; }
    STAmount const& outReq() const { return saOutReq; }

    STAmount const& inAct() const { return saInAct; }
    STAmount const& outAct() const { return saOutAct; }
    STAmount const& inReq() const { return saInReq; }

    void setInPass(STAmount const& sa)
    {
        saInPass = sa;
    }

    void setOutPass(STAmount const& sa)
    {
        saOutPass = sa;
    }

    AccountIssueToNodeIndex const& forward() { return umForward; }
    AccountIssueToNodeIndex const& reverse() { return umReverse; }

    void insertReverse (AccountIssue const& ai, path::NodeIndex i)
    {
        umReverse.insert({ai, i});
    }

    static char const* getCountedObjectName () { return "PathState"; }
    OfferIndexList& unfundedOffers() { return unfundedOffers_; }

    void setStatus(TER status) { terStatus = status; }
    TER status() const { return terStatus; }

    std::uint64_t quality() const { return uQuality; }
    void setQuality (std::uint64_t q) { uQuality = q; }

    void setIndex (int i) { mIndex  = i; }
    int index() const { return mIndex; }

    TER checkNoRipple (AccountID const& destinationAccountID,
                       AccountID const& sourceAccountID);
    void checkFreeze ();

    static bool lessPriority (PathState const& lhs, PathState const& rhs);

    PaymentSandbox&
    view()
    {
        return *view_;
    }

    void resetView (PaymentSandbox const& view)
    {
        view_.emplace(&view);
    }

    bool isDry() const
    {
        return !(saInPass && saOutPass);
    }

private:
    TER checkNoRipple (
        AccountID const&, AccountID const&, AccountID const&, Currency const&);

    
    void clear();

    TER pushNode (
        int const iType,
        AccountID const& account,
        Currency const& currency,
        AccountID const& issuer);

    TER pushImpliedNodes (
        AccountID const& account,
        Currency const& currency,
        AccountID const& issuer);

    Json::Value getJson () const;

private:
    boost::optional<PaymentSandbox> view_;

    int                         mIndex;    
    std::uint64_t               uQuality;  

    STAmount const&             saInReq;   
    STAmount                    saInAct;   
    STAmount                    saInPass;  

    STAmount const&             saOutReq;  
    STAmount                    saOutAct;  
    STAmount                    saOutPass; 

    TER terStatus;

    path::Node::List nodes_;

    OfferIndexList unfundedOffers_;

    AccountIssueToNodeIndex umForward;

    AccountIssueToNodeIndex umReverse;

    beast::Journal j_;
};

} 

#endif








