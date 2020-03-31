


#ifndef RIPPLE_CRYPTO_GENERATEDETERMINISTICKEY_H_INCLUDED
#define RIPPLE_CRYPTO_GENERATEDETERMINISTICKEY_H_INCLUDED

#include <ripple/basics/base_uint.h>
#include <ripple/basics/Blob.h>

namespace ripple {

Blob
generateRootDeterministicPublicKey (
    uint128 const& seed);

uint256
generateRootDeterministicPrivateKey (
    uint128 const& seed);

Blob
generatePublicDeterministicKey (
    Blob const& generator,
    int n);

uint256
generatePrivateDeterministicKey (
    Blob const& family,
    uint128 const& seed,
    int n);

} 

#endif








