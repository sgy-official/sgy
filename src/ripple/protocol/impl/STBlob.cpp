

#include <ripple/protocol/STBlob.h>
#include <ripple/basics/StringUtilities.h>

namespace ripple {

STBlob::STBlob (SerialIter& st, SField const& name)
    : STBase (name)
    , value_ (st.getVLBuffer ())
{
}

std::string
STBlob::getText () const
{
    return strHex (value_);
}

bool
STBlob::isEquivalent (const STBase& t) const
{
    const STBlob* v = dynamic_cast<const STBlob*> (&t);
    return v && (value_ == v->value_);
}

} 
























