

#include <ripple/protocol/InnerObjectFormats.h>

namespace ripple {

InnerObjectFormats::InnerObjectFormats ()
{
    add (sfSignerEntry.jsonName.c_str(), sfSignerEntry.getCode(),
        {
            {sfAccount,       soeREQUIRED},
            {sfSignerWeight,  soeREQUIRED},
        });

    add (sfSigner.jsonName.c_str(), sfSigner.getCode(),
        {
            {sfAccount,       soeREQUIRED},
            {sfSigningPubKey, soeREQUIRED},
            {sfTxnSignature,  soeREQUIRED},
        });
}

InnerObjectFormats const&
InnerObjectFormats::getInstance ()
{
    static InnerObjectFormats instance;
    return instance;
}

SOTemplate const*
InnerObjectFormats::findSOTemplateBySField (SField const& sField) const
{
    auto itemPtr = findByType (sField.getCode ());
    if (itemPtr)
        return &(itemPtr->getSOTemplate());

    return nullptr;
}

} 
























