

#include <ripple/protocol/STPathSet.h>
#include <ripple/protocol/jss.h>
#include <ripple/basics/contract.h>
#include <ripple/basics/Log.h>
#include <ripple/basics/strHex.h>
#include <ripple/basics/StringUtilities.h>
#include <cstddef>

namespace ripple {

std::size_t
STPathElement::get_hash (STPathElement const& element)
{
    std::size_t hash_account  = 2654435761;
    std::size_t hash_currency = 2654435761;
    std::size_t hash_issuer   = 2654435761;


    for (auto const x : element.getAccountID ())
        hash_account += (hash_account * 257) ^ x;

    for (auto const x : element.getCurrency ())
        hash_currency += (hash_currency * 509) ^ x;

    for (auto const x : element.getIssuerID ())
        hash_issuer += (hash_issuer * 911) ^ x;

    return (hash_account ^ hash_currency ^ hash_issuer);
}

STPathSet::STPathSet (SerialIter& sit, SField const& name)
    : STBase(name)
{
    std::vector<STPathElement> path;
    for(;;)
    {
        int iType = sit.get8 ();

        if (iType == STPathElement::typeNone ||
            iType == STPathElement::typeBoundary)
        {
            if (path.empty ())
            {
                JLOG (debugLog().error())
                    << "Empty path in pathset";
                Throw<std::runtime_error> ("empty path");
            }

            push_back (path);
            path.clear ();

            if (iType == STPathElement::typeNone)
                return;
        }
        else if (iType & ~STPathElement::typeAll)
        {
            JLOG (debugLog().error())
                << "Bad path element " << iType << " in pathset";
            Throw<std::runtime_error> ("bad path element");
        }
        else
        {
            auto hasAccount = iType & STPathElement::typeAccount;
            auto hasCurrency = iType & STPathElement::typeCurrency;
            auto hasIssuer = iType & STPathElement::typeIssuer;

            AccountID account;
            Currency currency;
            AccountID issuer;

            if (hasAccount)
                account = sit.get160();

            if (hasCurrency)
                currency = sit.get160();

            if (hasIssuer)
                issuer = sit.get160();

            path.emplace_back (account, currency, issuer, hasCurrency);
        }
    }
}

bool
STPathSet::assembleAdd(STPath const& base, STPathElement const& tail)
{ 
    value.push_back (base);

    std::vector<STPath>::reverse_iterator it = value.rbegin ();

    STPath& newPath = *it;
    newPath.push_back (tail);

    while (++it != value.rend ())
    {
        if (*it == newPath)
        {
            value.pop_back ();
            return false;
        }
    }
    return true;
}

bool
STPathSet::isEquivalent (const STBase& t) const
{
    const STPathSet* v = dynamic_cast<const STPathSet*> (&t);
    return v && (value == v->value);
}

bool
STPath::hasSeen (
    AccountID const& account, Currency const& currency,
    AccountID const& issuer) const
{
    for (auto& p: mPath)
    {
        if (p.getAccountID () == account
            && p.getCurrency () == currency
            && p.getIssuerID () == issuer)
            return true;
    }

    return false;
}

Json::Value
STPath::getJson (JsonOptions) const
{
    Json::Value ret (Json::arrayValue);

    for (auto it: mPath)
    {
        Json::Value elem (Json::objectValue);
        auto const iType   = it.getNodeType ();

        elem[jss::type]      = iType;
        elem[jss::type_hex]  = strHex (iType);

        if (iType & STPathElement::typeAccount)
            elem[jss::account]  = to_string (it.getAccountID ());

        if (iType & STPathElement::typeCurrency)
            elem[jss::currency] = to_string (it.getCurrency ());

        if (iType & STPathElement::typeIssuer)
            elem[jss::issuer]   = to_string (it.getIssuerID ());

        ret.append (elem);
    }

    return ret;
}

Json::Value
STPathSet::getJson (JsonOptions options) const
{
    Json::Value ret (Json::arrayValue);
    for (auto it: value)
        ret.append (it.getJson (options));

    return ret;
}

void
STPathSet::add (Serializer& s) const
{
    assert (fName->isBinary ());
    assert (fName->fieldType == STI_PATHSET);
    bool first = true;

    for (auto const& spPath : value)
    {
        if (!first)
            s.add8 (STPathElement::typeBoundary);

        for (auto const& speElement : spPath)
        {
            int iType = speElement.getNodeType ();

            s.add8 (iType);

            if (iType & STPathElement::typeAccount)
                s.add160 (speElement.getAccountID ());

            if (iType & STPathElement::typeCurrency)
                s.add160 (speElement.getCurrency ());

            if (iType & STPathElement::typeIssuer)
                s.add160 (speElement.getIssuerID ());
        }

        first = false;
    }

    s.add8 (STPathElement::typeNone);
}

} 
























