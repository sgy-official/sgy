

#include <ripple/basics/Log.h>
#include <ripple/basics/StringUtilities.h>
#include <ripple/protocol/jss.h>
#include <ripple/protocol/STVector256.h>

namespace ripple {

STVector256::STVector256(SerialIter& sit, SField const& name)
    : STBase(name)
{
    Blob data = sit.getVL ();
    auto const count = data.size () / (256 / 8);
    mValue.reserve (count);
    Blob::iterator begin = data.begin ();
    unsigned int uStart  = 0;
    for (unsigned int i = 0; i != count; i++)
    {
        unsigned int uEnd = uStart + (256 / 8);
        mValue.push_back (uint256 (Blob (begin + uStart, begin + uEnd)));
        uStart  = uEnd;
    }
}

void
STVector256::add (Serializer& s) const
{
    assert (fName->isBinary ());
    assert (fName->fieldType == STI_VECTOR256);
    s.addVL (mValue.begin(), mValue.end(), mValue.size () * (256 / 8));
}

bool
STVector256::isEquivalent (const STBase& t) const
{
    const STVector256* v = dynamic_cast<const STVector256*> (&t);
    return v && (mValue == v->mValue);
}

Json::Value
STVector256::getJson (JsonOptions) const
{
    Json::Value ret (Json::arrayValue);

    for (auto const& vEntry : mValue)
        ret.append (to_string (vEntry));

    return ret;
}

} 
























