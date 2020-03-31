

#ifndef _SECP256K1_ECMULT_GEN_
#define _SECP256K1_ECMULT_GEN_

#include "scalar.h"
#include "group.h"

typedef struct {
    
    secp256k1_ge_storage (*prec)[64][16]; 
    secp256k1_scalar blind;
    secp256k1_gej initial;
} secp256k1_ecmult_gen_context;

static void secp256k1_ecmult_gen_context_init(secp256k1_ecmult_gen_context* ctx);
static void secp256k1_ecmult_gen_context_build(secp256k1_ecmult_gen_context* ctx, const secp256k1_callback* cb);
static void secp256k1_ecmult_gen_context_clone(secp256k1_ecmult_gen_context *dst,
                                               const secp256k1_ecmult_gen_context* src, const secp256k1_callback* cb);
static void secp256k1_ecmult_gen_context_clear(secp256k1_ecmult_gen_context* ctx);
static int secp256k1_ecmult_gen_context_is_built(const secp256k1_ecmult_gen_context* ctx);


static void secp256k1_ecmult_gen(const secp256k1_ecmult_gen_context* ctx, secp256k1_gej *r, const secp256k1_scalar *a);

static void secp256k1_ecmult_gen_blind(secp256k1_ecmult_gen_context *ctx, const unsigned char *seed32);

#endif








