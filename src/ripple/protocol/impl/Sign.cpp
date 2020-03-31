

#include <ripple/protocol/Sign.h>

namespace ripple {

void
sign (STObject& st, HashPrefix const& prefix,
    KeyType type, SecretKey const& sk,
    SF_Blob const& sigField)
{
    Serializer ss;
    ss.add32(prefix);
    st.addWithoutSigningFields(ss);
    set(st, sigField,
        sign(type, sk, ss.slice()));
}

bool
verify (STObject const& st, HashPrefix const& prefix,
    PublicKey const& pk, SF_Blob const& sigField)
{
    auto const sig = get(st, sigField);
    if (! sig)
        return false;
    Serializer ss;
    ss.add32(prefix);
    st.addWithoutSigningFields(ss);
    return verify(pk,
        Slice(ss.data(), ss.size()),
            Slice(sig->data(), sig->size()));
}

Serializer
buildMultiSigningData (STObject const& obj, AccountID const& signingID)
{
    Serializer s {startMultiSigningData (obj)};
    finishMultiSigningData (signingID, s);
    return s;
}

Serializer
startMultiSigningData (STObject const& obj)
{
    Serializer s;
    s.add32 (HashPrefix::txMultiSign);
    obj.addWithoutSigningFields (s);
    return s;
}

} 
























