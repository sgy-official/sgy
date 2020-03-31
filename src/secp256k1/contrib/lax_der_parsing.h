





#ifndef _SECP256K1_CONTRIB_LAX_DER_PARSING_H_
#define _SECP256K1_CONTRIB_LAX_DER_PARSING_H_

#include <secp256k1.h>

# ifdef __cplusplus
extern "C" {
# endif


int ecdsa_signature_parse_der_lax(
    const secp256k1_context* ctx,
    secp256k1_ecdsa_signature* sig,
    const unsigned char *input,
    size_t inputlen
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

#ifdef __cplusplus
}
#endif

#endif








