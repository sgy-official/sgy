

#include <ripple/protocol/STBase.h>
#include <boost/checked_delete.hpp>
#include <cassert>
#include <memory>

namespace ripple {

STBase::STBase()
    : fName(&sfGeneric)
{
}

STBase::STBase (SField const& n)
    : fName(&n)
{
    assert(fName);
}

STBase&
STBase::operator= (const STBase& t)
{
    if ((t.fName != fName) && fName->isUseful() && t.fName->isUseful())
    {
        
    }
    if (!fName->isUseful())
        fName = t.fName;
    return *this;
}

bool
STBase::operator== (const STBase& t) const
{
    return (getSType () == t.getSType ()) && isEquivalent (t);
}

bool
STBase::operator!= (const STBase& t) const
{
    return (getSType () != t.getSType ()) || !isEquivalent (t);
}

SerializedTypeID
STBase::getSType() const
{
    return STI_NOTPRESENT;
}

std::string
STBase::getFullText() const
{
    std::string ret;

    if (getSType () != STI_NOTPRESENT)
    {
        if (fName->hasName ())
        {
            ret = fName->fieldName;
            ret += " = ";
        }

        ret += getText ();
    }

    return ret;
}

std::string
STBase::getText() const
{
    return std::string();
}

Json::Value
STBase::getJson (JsonOptions ) const
{
    return getText();
}

void
STBase::add (Serializer& s) const
{
    assert(false);
}

bool
STBase::isEquivalent (const STBase& t) const
{
    assert (getSType () == STI_NOTPRESENT);
    if (t.getSType () == STI_NOTPRESENT)
        return true;
    return false;
}

bool
STBase::isDefault() const
{
    return true;
}

void
STBase::setFName (SField const& n)
{
    fName = &n;
    assert (fName);
}

SField const&
STBase::getFName() const
{
    return *fName;
}

void
STBase::addFieldID (Serializer& s) const
{
    assert (fName->isBinary ());
    s.addFieldID (fName->fieldType, fName->fieldValue);
}


std::ostream&
operator<< (std::ostream& out, const STBase& t)
{
    return out << t.getFullText ();
}

} 
























