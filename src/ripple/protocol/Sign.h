

#ifndef RIPPLE_PROTOCOL_SIGN_H_INCLUDED
#define RIPPLE_PROTOCOL_SIGN_H_INCLUDED

#include <ripple/protocol/HashPrefix.h>
#include <ripple/protocol/PublicKey.h>
#include <ripple/protocol/SecretKey.h>
#include <ripple/protocol/STObject.h>
#include <utility>

namespace ripple {


void
sign (STObject& st, HashPrefix const& prefix,
    KeyType type, SecretKey const& sk,
        SF_Blob const& sigField = sfSignature);


bool
verify (STObject const& st, HashPrefix const& prefix,
    PublicKey const& pk,
        SF_Blob const& sigField = sfSignature);


Serializer
buildMultiSigningData (STObject const& obj, AccountID const& signingID);


Serializer
startMultiSigningData (STObject const& obj);

inline void
finishMultiSigningData (AccountID const& signingID, Serializer& s)
{
    s.add160 (signingID);
}

} 

#endif








