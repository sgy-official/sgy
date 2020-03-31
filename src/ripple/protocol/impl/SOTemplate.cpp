

#include <ripple/protocol/SOTemplate.h>

namespace ripple {

SOTemplate::SOTemplate (
    std::initializer_list<SOElement> uniqueFields,
    std::initializer_list<SOElement> commonFields)
    : indices_ (SField::getNumFields () + 1, -1) 
{
    elements_.reserve (uniqueFields.size() + commonFields.size());
    elements_.assign (uniqueFields);
    elements_.insert (elements_.end(), commonFields);

    for (std::size_t i = 0; i < elements_.size(); ++i)
    {
        SField const& sField {elements_[i].sField()};

        if (sField.getNum() <= 0 || sField.getNum() >= indices_.size())
            Throw<std::runtime_error> ("Invalid field index for SOTemplate.");

        if (getIndex (sField) != -1)
            Throw<std::runtime_error> ("Duplicate field index for SOTemplate.");

        indices_[sField.getNum ()] = i;
    }
}

int SOTemplate::getIndex (SField const& sField) const
{
    if (sField.getNum() <= 0 || sField.getNum() >= indices_.size())
        Throw<std::runtime_error> ("Invalid field index for getIndex().");

    return indices_[sField.getNum()];
}

} 
























