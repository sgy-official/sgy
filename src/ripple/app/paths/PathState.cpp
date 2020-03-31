

#include <ripple/app/paths/Credit.h>
#include <ripple/app/paths/PathState.h>
#include <ripple/basics/Log.h>
#include <ripple/json/to_string.h>
#include <ripple/protocol/Indexes.h>
#include <ripple/protocol/jss.h>
#include <boost/lexical_cast.hpp>

namespace ripple {


class RippleCalc; 

void PathState::clear()
{
    saInPass = saInReq.zeroed();
    saOutPass = saOutReq.zeroed();
    unfundedOffers_.clear ();
    umReverse.clear ();

    for (auto& node: nodes_)
        node.clear();
}

void PathState::reset(STAmount const& in, STAmount const& out)
{
    clear();

    saInAct = in;
    saOutAct = out;

    if (inReq() > beast::zero && inAct() >= inReq())
    {
        JLOG (j_.warn())
            <<  "rippleCalc: DONE:"
            << " inAct()=" << inAct()
            << " inReq()=" << inReq();
    }

    assert (inReq() < beast::zero || inAct() < inReq());

    if (outAct() >= outReq())
    {
        JLOG (j_.warn())
            << "rippleCalc: ALREADY DONE:"
            << " saOutAct=" << outAct()
            << " saOutReq=" << outReq();
    }

    assert(outAct() < outReq());
    assert (nodes().size () >= 2);
}

bool PathState::lessPriority (PathState const& lhs, PathState const& rhs)
{
    if (lhs.uQuality != rhs.uQuality)
        return lhs.uQuality > rhs.uQuality;     

    if (lhs.saOutPass != rhs.saOutPass)
        return lhs.saOutPass < rhs.saOutPass;   

    return lhs.mIndex > rhs.mIndex;             
}


TER PathState::pushImpliedNodes (
    AccountID const& account,    
    Currency const& currency,  
    AccountID const& issuer)    
{
    TER resultCode = tesSUCCESS;

     JLOG (j_.trace()) << "pushImpliedNodes>" <<
         " " << account <<
         " " << currency <<
         " " << issuer;

    if (nodes_.back ().issue_.currency != currency)
    {


        auto type = isXRP(currency) ? STPathElement::typeCurrency
            : STPathElement::typeCurrency | STPathElement::typeIssuer;

        resultCode = pushNode (type, xrpAccount(), currency, issuer);
    }


    if (resultCode == tesSUCCESS
        && !isXRP(currency)
        && nodes_.back ().account_ != issuer
        && account != issuer)
    {
        resultCode = pushNode (
            STPathElement::typeAll, issuer, currency, issuer);
    }

    JLOG (j_.trace())
        << "pushImpliedNodes< : " << transToken (resultCode);

    return resultCode;
}

TER PathState::pushNode (
    const int iType,
    AccountID const& account,    
    Currency const& currency,  
    AccountID const& issuer)     
{
    path::Node node;
    const bool pathIsEmpty = nodes_.empty ();


    auto const& backNode = pathIsEmpty ? path::Node () : nodes_.back ();

    const bool hasAccount = (iType & STPathElement::typeAccount);

    const bool hasCurrency = (iType & STPathElement::typeCurrency);

    const bool hasIssuer = (iType & STPathElement::typeIssuer);

    TER resultCode = tesSUCCESS;

    JLOG (j_.trace())
         << "pushNode> " << iType << ": "
         << (hasAccount ? to_string(account) : std::string("-")) << " "
         << (hasCurrency ? to_string(currency) : std::string("-")) << "/"
         << (hasIssuer ? to_string(issuer) : std::string("-")) << "/";

    node.uFlags = iType;
    node.issue_.currency = hasCurrency ?
            currency : backNode.issue_.currency;


    if (iType & ~STPathElement::typeAll)
    {
        JLOG (j_.debug()) << "pushNode: bad bits.";
        resultCode = temBAD_PATH;
    }
    else if (hasIssuer && isXRP (node.issue_))
    {
        JLOG (j_.debug()) << "pushNode: issuer specified for XRP.";

        resultCode = temBAD_PATH;
    }
    else if (hasIssuer && !issuer)
    {
        JLOG (j_.debug()) << "pushNode: specified bad issuer.";

        resultCode = temBAD_PATH;
    }
    else if (!hasAccount && !hasCurrency && !hasIssuer)
    {
        JLOG (j_.debug())
            << "pushNode: offer must specify at least currency or issuer.";
        resultCode = temBAD_PATH;
    }
    else if (hasAccount)
    {
        node.account_ = account;
        node.issue_.account = hasIssuer ? issuer :
                (isXRP (node.issue_) ? xrpAccount() : account);
        node.saRevRedeem = STAmount ({node.issue_.currency, account});
        node.saRevIssue = node.saRevRedeem;

        node.saRevDeliver = STAmount (node.issue_);
        node.saFwdDeliver = node.saRevDeliver;

        if (pathIsEmpty)
        {
        }
        else if (!account)
        {
            JLOG (j_.debug())
                << "pushNode: specified bad account.";
            resultCode = temBAD_PATH;
        }
        else
        {
            JLOG (j_.trace())
                << "pushNode: imply for account.";

            resultCode = pushImpliedNodes (
                node.account_,
                node.issue_.currency,
                isXRP(node.issue_.currency) ? xrpAccount() : account);

        }

        if (resultCode == tesSUCCESS && !nodes_.empty ())
        {
            auto const& backNode = nodes_.back ();
            if (backNode.isAccount())
            {
                auto sleRippleState = view().peek(
                    keylet::line(backNode.account_, node.account_, backNode.issue_.currency));

                if (!sleRippleState)
                {
                    JLOG (j_.trace())
                            << "pushNode: No credit line between "
                            << backNode.account_ << " and " << node.account_
                            << " for " << node.issue_.currency << "." ;

                    JLOG (j_.trace()) << getJson ();

                    resultCode   = terNO_LINE;
                }
                else
                {
                    JLOG (j_.trace())
                            << "pushNode: Credit line found between "
                            << backNode.account_ << " and " << node.account_
                            << " for " << node.issue_.currency << "." ;

                    auto sleBck  = view().peek (
                        keylet::account(backNode.account_));
                    bool bHigh = backNode.account_ > node.account_;

                    if (!sleBck)
                    {
                        JLOG (j_.warn())
                            << "pushNode: delay: can't receive IOUs from "
                            << "non-existent issuer: " << backNode.account_;

                        resultCode   = terNO_ACCOUNT;
                    }
                    else if ((sleBck->getFieldU32 (sfFlags) & lsfRequireAuth) &&
                             !(sleRippleState->getFieldU32 (sfFlags) &
                                  (bHigh ? lsfHighAuth : lsfLowAuth)) &&
                             sleRippleState->getFieldAmount(sfBalance) == beast::zero)
                    {
                        JLOG (j_.warn())
                                << "pushNode: delay: can't receive IOUs from "
                                << "issuer without auth.";

                        resultCode   = terNO_AUTH;
                    }

                    if (resultCode == tesSUCCESS)
                    {
                        STAmount saOwed = creditBalance (view(),
                            node.account_, backNode.account_,
                            node.issue_.currency);
                        STAmount saLimit;

                        if (saOwed <= beast::zero)
                        {
                            saLimit = creditLimit (view(),
                                node.account_,
                                backNode.account_,
                                node.issue_.currency);
                            if (-saOwed >= saLimit)
                            {
                                JLOG (j_.debug()) <<
                                    "pushNode: dry:" <<
                                    " saOwed=" << saOwed <<
                                    " saLimit=" << saLimit;

                                resultCode   = tecPATH_DRY;
                            }
                        }
                    }
                }
            }
        }

        if (resultCode == tesSUCCESS)
            nodes_.push_back (node);
    }
    else
    {
        if (hasIssuer)
            node.issue_.account = issuer;
        else if (isXRP (node.issue_.currency))
            node.issue_.account = xrpAccount();
        else if (isXRP (backNode.issue_.account))
            node.issue_.account = backNode.account_;
        else
            node.issue_.account = backNode.issue_.account;

        node.saRevDeliver = STAmount (node.issue_);
        node.saFwdDeliver = node.saRevDeliver;

        if (!isConsistent (node.issue_))
        {
            JLOG (j_.debug())
                << "pushNode: currency is inconsistent with issuer.";

            resultCode = temBAD_PATH;
        }
        else if (backNode.issue_ == node.issue_)
        {
            JLOG (j_.debug()) <<
                "pushNode: bad path: offer to same currency and issuer";
            resultCode = temBAD_PATH;
        }
        else {
            JLOG (j_.trace()) << "pushNode: imply for offer.";

            resultCode   = pushImpliedNodes (
                xrpAccount(), 
                backNode.issue_.currency,
                backNode.issue_.account);
        }

        if (resultCode == tesSUCCESS)
            nodes_.push_back (node);
    }

    JLOG (j_.trace()) << "pushNode< : " << transToken (resultCode);
    return resultCode;
}

TER PathState::expandPath (
    STPath const& spSourcePath,
    AccountID const& uReceiverID,
    AccountID const& uSenderID)
{
    uQuality = 1;            

    Currency const& uMaxCurrencyID = saInReq.getCurrency ();
    AccountID const& uMaxIssuerID = saInReq.getIssuer ();

    Currency const& currencyOutID = saOutReq.getCurrency ();
    AccountID const& issuerOutID = saOutReq.getIssuer ();
    AccountID const& uSenderIssuerID
        = isXRP(uMaxCurrencyID) ? xrpAccount() : uSenderID;

    JLOG (j_.trace())
        << "expandPath> " << spSourcePath.getJson (JsonOptions::none);

    terStatus = tesSUCCESS;

    if ((isXRP (uMaxCurrencyID) && !isXRP (uMaxIssuerID))
        || (isXRP (currencyOutID) && !isXRP (issuerOutID)))
    {
        JLOG (j_.debug())
            << "expandPath> issuer with XRP";
        terStatus   = temBAD_PATH;
    }

    if (terStatus == tesSUCCESS)
    {
        terStatus   = pushNode (
            !isXRP(uMaxCurrencyID)
            ? STPathElement::typeAccount | STPathElement::typeCurrency |
              STPathElement::typeIssuer
            : STPathElement::typeAccount | STPathElement::typeCurrency,
            uSenderID,
            uMaxCurrencyID, 
            uSenderIssuerID);
    }

    JLOG (j_.debug())
        << "expandPath: pushed:"
        << " account=" << uSenderID
        << " currency=" << uMaxCurrencyID
        << " issuer=" << uSenderIssuerID;

    if (tesSUCCESS == terStatus && uMaxIssuerID != uSenderIssuerID)
    {

        const auto uNxtCurrencyID  = spSourcePath.size ()
                ? Currency(spSourcePath.front ().getCurrency ())
                : currencyOutID;

        const auto nextAccountID   = spSourcePath.size ()
                ? AccountID(spSourcePath. front ().getAccountID ())
                : !isXRP(currencyOutID)
                ? (issuerOutID == uReceiverID)
                ? AccountID(uReceiverID)
                : AccountID(issuerOutID)                      
                : xrpAccount();

        JLOG (j_.debug())
            << "expandPath: implied check:"
            << " uMaxIssuerID=" << uMaxIssuerID
            << " uSenderIssuerID=" << uSenderIssuerID
            << " uNxtCurrencyID=" << uNxtCurrencyID
            << " nextAccountID=" << nextAccountID;

        if (!uNxtCurrencyID
            || uMaxCurrencyID != uNxtCurrencyID
            || uMaxIssuerID != nextAccountID)
        {
            JLOG (j_.debug())
                << "expandPath: sender implied:"
                << " account=" << uMaxIssuerID
                << " currency=" << uMaxCurrencyID
                << " issuer=" << uMaxIssuerID;

            terStatus = pushNode (
                !isXRP(uMaxCurrencyID)
                    ? STPathElement::typeAccount | STPathElement::typeCurrency |
                      STPathElement::typeIssuer
                    : STPathElement::typeAccount | STPathElement::typeCurrency,
                uMaxIssuerID,
                uMaxCurrencyID,
                uMaxIssuerID);
        }
    }

    for (auto & speElement: spSourcePath)
    {
        if (terStatus == tesSUCCESS)
        {
            JLOG (j_.trace()) << "expandPath: element in path";
            terStatus = pushNode (
                speElement.getNodeType (), speElement.getAccountID (),
                speElement.getCurrency (), speElement.getIssuerID ());
        }
    }

    if (terStatus == tesSUCCESS
        && !isXRP(currencyOutID)               
        && issuerOutID != uReceiverID)         
    {
        assert (!nodes_.empty ());

        auto const& backNode = nodes_.back ();

        if (backNode.issue_.currency != currencyOutID  
            || backNode.account_ != issuerOutID)       
        {
            JLOG (j_.debug())
                << "expandPath: receiver implied:"
                << " account=" << issuerOutID
                << " currency=" << currencyOutID
                << " issuer=" << issuerOutID;

            terStatus   = pushNode (
                !isXRP(currencyOutID)
                    ? STPathElement::typeAccount | STPathElement::typeCurrency |
                      STPathElement::typeIssuer
                    : STPathElement::typeAccount | STPathElement::typeCurrency,
                issuerOutID,
                currencyOutID,
                issuerOutID);
        }
    }

    if (terStatus == tesSUCCESS)
    {

        terStatus   = pushNode (
            !isXRP(currencyOutID)
                ? STPathElement::typeAccount | STPathElement::typeCurrency |
                   STPathElement::typeIssuer
               : STPathElement::typeAccount | STPathElement::typeCurrency,
            uReceiverID,                                    
            currencyOutID,                                 
            uReceiverID);
    }

    if (terStatus == tesSUCCESS)
    {
        unsigned int index = 0;
        for (auto& node: nodes_)
        {
            AccountIssue accountIssue (node.account_, node.issue_);
            if (!umForward.insert ({accountIssue, index++}).second)
            {
                JLOG (j_.debug()) <<
                    "expandPath: loop detected: " << getJson ();

                terStatus = temBAD_PATH_LOOP;
                break;
            }
        }
    }

    JLOG (j_.trace())
        << "expandPath:"
        << " in=" << uMaxCurrencyID
        << "/" << uMaxIssuerID
        << " out=" << currencyOutID
        << "/" << issuerOutID
        << ": " << getJson ();
    return terStatus;
}



void PathState::checkFreeze()
{
    assert (nodes_.size() >= 2);

    if (nodes_.size() == 2)
        return;

    SLE::pointer sle;

    for (std::size_t i = 0; i < (nodes_.size() - 1); ++i)
    {
        if (nodes_[i].uFlags & STPathElement::typeIssuer)
        {
            sle = view().peek (keylet::account(nodes_[i].issue_.account));

            if (sle && sle->isFlag (lsfGlobalFreeze))
            {
                terStatus = terNO_LINE;
                return;
            }
        }

        if (nodes_[i].uFlags & STPathElement::typeAccount)
        {
            Currency const& currencyID = nodes_[i].issue_.currency;
            AccountID const& inAccount = nodes_[i].account_;
            AccountID const& outAccount = nodes_[i+1].account_;

            if (inAccount != outAccount)
            {
                sle = view().peek (keylet::account(outAccount));

                if (sle && sle->isFlag (lsfGlobalFreeze))
                {
                    terStatus = terNO_LINE;
                    return;
                }

                sle = view().peek (keylet::line(inAccount,
                        outAccount, currencyID));

                if (sle && sle->isFlag (
                    (outAccount > inAccount) ? lsfHighFreeze : lsfLowFreeze))
                {
                    terStatus = terNO_LINE;
                    return;
                }
            }
        }
    }
}


TER PathState::checkNoRipple (
    AccountID const& firstAccount,
    AccountID const& secondAccount,
    AccountID const& thirdAccount,
    Currency const& currency)
{
    SLE::pointer sleIn = view().peek (
        keylet::line(firstAccount, secondAccount, currency));
    SLE::pointer sleOut = view().peek (
        keylet::line(secondAccount, thirdAccount, currency));

    if (!sleIn || !sleOut)
    {
        terStatus = terNO_LINE;
    }
    else if (
        sleIn->getFieldU32 (sfFlags) &
            ((secondAccount > firstAccount) ? lsfHighNoRipple : lsfLowNoRipple) &&
        sleOut->getFieldU32 (sfFlags) &
            ((secondAccount > thirdAccount) ? lsfHighNoRipple : lsfLowNoRipple))
    {
        JLOG (j_.info())
            << "Path violates noRipple constraint between "
            << firstAccount << ", "
            << secondAccount << " and "
            << thirdAccount;

        terStatus = terNO_RIPPLE;
    }
    return terStatus;
}

TER PathState::checkNoRipple (
    AccountID const& uDstAccountID,
    AccountID const& uSrcAccountID)
{
    if (nodes_.size() == 0)
       return terStatus;

    if (nodes_.size() == 1)
    {
        if (nodes_[0].isAccount() &&
            (nodes_[0].account_ != uSrcAccountID) &&
            (nodes_[0].account_ != uDstAccountID))
        {
            if (saInReq.getCurrency() != saOutReq.getCurrency())
            {
                terStatus = terNO_LINE;
            }
            else
            {
                terStatus = checkNoRipple (
                    uSrcAccountID, nodes_[0].account_, uDstAccountID,
                    nodes_[0].issue_.currency);
            }
        }
        return terStatus;
    }

    if (nodes_[0].isAccount() &&
        nodes_[1].isAccount() &&
        (nodes_[0].account_ != uSrcAccountID))
    {
        if ((nodes_[0].issue_.currency != nodes_[1].issue_.currency))
        {
            terStatus = terNO_LINE;
            return terStatus;
        }
        else
        {
            terStatus = checkNoRipple (
                uSrcAccountID, nodes_[0].account_, nodes_[1].account_,
                nodes_[0].issue_.currency);
            if (terStatus != tesSUCCESS)
                return terStatus;
        }
    }

    size_t s = nodes_.size() - 2;
    if (nodes_[s].isAccount() &&
        nodes_[s + 1].isAccount() &&
        (uDstAccountID != nodes_[s+1].account_))
    {
        if ((nodes_[s].issue_.currency != nodes_[s+1].issue_.currency))
        {
            terStatus = terNO_LINE;
            return terStatus;
        }
        else
        {
            terStatus = checkNoRipple (
                nodes_[s].account_, nodes_[s+1].account_,
                uDstAccountID, nodes_[s].issue_.currency);
            if (tesSUCCESS != terStatus)
                return terStatus;
        }
    }

    for (int i = 1; i < nodes_.size() - 1; ++i)
    {
        if (nodes_[i - 1].isAccount() &&
            nodes_[i].isAccount() &&
            nodes_[i + 1].isAccount())
        { 

            auto const& currencyID = nodes_[i].issue_.currency;
            if ((nodes_[i-1].issue_.currency != currencyID) ||
                (nodes_[i+1].issue_.currency != currencyID))
            {
                terStatus = temBAD_PATH;
                return terStatus;
            }
            terStatus = checkNoRipple (
                nodes_[i-1].account_, nodes_[i].account_, nodes_[i+1].account_,
                currencyID);
            if (terStatus != tesSUCCESS)
                return terStatus;
        }

        if (!nodes_[i - 1].isAccount() &&
            nodes_[i].isAccount() &&
            nodes_[i + 1].isAccount() &&
            nodes_[i -1].issue_.account != nodes_[i].account_)
        { 
            auto const& currencyID = nodes_[i].issue_.currency;
            terStatus = checkNoRipple (
                nodes_[i-1].issue_.account, nodes_[i].account_, nodes_[i+1].account_,
                currencyID);
            if (terStatus != tesSUCCESS)
                return terStatus;
        }
    }

    return tesSUCCESS;
}

Json::Value PathState::getJson () const
{
    Json::Value jvPathState (Json::objectValue);
    Json::Value jvNodes (Json::arrayValue);

    for (auto const &pnNode: nodes_)
        jvNodes.append (pnNode.getJson ());

    jvPathState[jss::status]   = terStatus;
    jvPathState[jss::index]    = mIndex;
    jvPathState[jss::nodes]    = jvNodes;

    if (saInReq)
        jvPathState["in_req"]   = saInReq.getJson (JsonOptions::none);

    if (saInAct)
        jvPathState["in_act"]   = saInAct.getJson (JsonOptions::none);

    if (saInPass)
        jvPathState["in_pass"]  = saInPass.getJson (JsonOptions::none);

    if (saOutReq)
        jvPathState["out_req"]  = saOutReq.getJson (JsonOptions::none);

    if (saOutAct)
        jvPathState["out_act"]  = saOutAct.getJson (JsonOptions::none);

    if (saOutPass)
        jvPathState["out_pass"] = saOutPass.getJson (JsonOptions::none);

    if (uQuality)
        jvPathState["uQuality"] = boost::lexical_cast<std::string>(uQuality);

    return jvPathState;
}

} 
























