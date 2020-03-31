

#ifndef RIPPLE_APP_PATHS_RIPPLECALC_H_INCLUDED
#define RIPPLE_APP_PATHS_RIPPLECALC_H_INCLUDED

#include <ripple/ledger/PaymentSandbox.h>
#include <ripple/app/paths/PathState.h>
#include <ripple/basics/Log.h>
#include <ripple/protocol/STAmount.h>
#include <ripple/protocol/TER.h>

#include <boost/container/flat_set.hpp>

namespace ripple {
class Config;
namespace path {

namespace detail {
struct FlowDebugInfo;
}


class RippleCalc
{
public:
    struct Input
    {
        explicit Input() = default;

        bool partialPaymentAllowed = false;
        bool defaultPathsAllowed = true;
        bool limitQuality = false;
        bool isLedgerOpen = true;
    };
    struct Output
    {
        explicit Output() = default;

        STAmount actualAmountIn;

        STAmount actualAmountOut;

        boost::container::flat_set<uint256> removableOffers;
    private:
        TER calculationResult_ = temUNKNOWN;

    public:
        TER result () const
        {
            return calculationResult_;
        }
        void setResult (TER const value)
        {
            calculationResult_ = value;
        }

    };

    static
    Output
    rippleCalculate(
        PaymentSandbox& view,


        STAmount const& saMaxAmountReq,             

        STAmount const& saDstAmountReq,

        AccountID const& uDstAccountID,
        AccountID const& uSrcAccountID,

        STPathSet const& spsPaths,
        Logs& l,
        Input const* const pInputs = nullptr);

    PaymentSandbox& view;

    boost::container::flat_set<uint256> permanentlyUnfundedOffers_;


    AccountIssueToNodeIndex mumSource_;
    beast::Journal j_;
    Logs& logs_;

private:
    RippleCalc (
        PaymentSandbox& view_,
        STAmount const& saMaxAmountReq,             
        STAmount const& saDstAmountReq,

        AccountID const& uDstAccountID,
        AccountID const& uSrcAccountID,
        STPathSet const& spsPaths,
        Logs& l)
            : view (view_),
              j_ (l.journal ("RippleCalc")),
              logs_ (l),
              saDstAmountReq_(saDstAmountReq),
              saMaxAmountReq_(saMaxAmountReq),
              uDstAccountID_(uDstAccountID),
              uSrcAccountID_(uSrcAccountID),
              spsPaths_(spsPaths)
    {
    }

    
    TER rippleCalculate (detail::FlowDebugInfo* flowDebugInfo=nullptr);

    
    bool addPathState(STPath const&, TER&);

    STAmount const& saDstAmountReq_;
    STAmount const& saMaxAmountReq_;
    AccountID const& uDstAccountID_;
    AccountID const& uSrcAccountID_;
    STPathSet const& spsPaths_;

    STAmount actualAmountIn_;

    STAmount actualAmountOut_;

    PathState::List pathStateList_;

    Input inputFlags;
};

} 
} 

#endif








