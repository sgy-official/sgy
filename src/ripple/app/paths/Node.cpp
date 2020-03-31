

#include <ripple/app/paths/Node.h>
#include <ripple/app/paths/PathState.h>
#include <ripple/protocol/jss.h>

namespace ripple {
namespace path {

bool Node::operator== (const Node& other) const
{
    return other.uFlags == uFlags
            && other.account_ == account_
            && other.issue_ == issue_;
}

Json::Value Node::getJson () const
{
    Json::Value jvNode (Json::objectValue);
    Json::Value jvFlags (Json::arrayValue);

    jvNode[jss::type]  = uFlags;

    bool const hasCurrency = !isXRP (issue_.currency);
    bool const hasAccount = !isXRP (account_);
    bool const hasIssuer = !isXRP (issue_.account);

    if (isAccount() || hasAccount)
        jvFlags.append (!isAccount() == hasAccount ? "account" : "-account");

    if (uFlags & STPathElement::typeCurrency || hasCurrency)
    {
        jvFlags.append ((uFlags & STPathElement::typeCurrency) && hasCurrency
            ? "currency"
            : "-currency");
    }

    if (uFlags & STPathElement::typeIssuer || hasIssuer)
    {
        jvFlags.append ((uFlags & STPathElement::typeIssuer) && hasIssuer
            ? "issuer"
            : "-issuer");
    }

    jvNode["flags"] = jvFlags;

    if (!isXRP (account_))
        jvNode[jss::account] = to_string (account_);

    if (!isXRP (issue_.currency))
        jvNode[jss::currency] = to_string (issue_.currency);

    if (!isXRP (issue_.account))
        jvNode[jss::issuer] = to_string (issue_.account);

    if (saRevRedeem)
        jvNode["rev_redeem"] = saRevRedeem.getFullText ();

    if (saRevIssue)
        jvNode["rev_issue"] = saRevIssue.getFullText ();

    if (saRevDeliver)
        jvNode["rev_deliver"] = saRevDeliver.getFullText ();

    if (saFwdRedeem)
        jvNode["fwd_redeem"] = saFwdRedeem.getFullText ();

    if (saFwdIssue)
        jvNode["fwd_issue"] = saFwdIssue.getFullText ();

    if (saFwdDeliver)
        jvNode["fwd_deliver"] = saFwdDeliver.getFullText ();

    return jvNode;
}

} 
} 
























