

#ifndef _SECP256K1_FIELD_
#define _SECP256K1_FIELD_



#if defined HAVE_CONFIG_H
#include "libsecp256k1-config.h"
#endif

#if defined(USE_FIELD_10X26)
#include "field_10x26.h"
#elif defined(USE_FIELD_5X52)
#include "field_5x52.h"
#else
#error "Please select field implementation"
#endif

#include "util.h"


static void secp256k1_fe_normalize(secp256k1_fe *r);


static void secp256k1_fe_normalize_weak(secp256k1_fe *r);


static void secp256k1_fe_normalize_var(secp256k1_fe *r);


static int secp256k1_fe_normalizes_to_zero(secp256k1_fe *r);


static int secp256k1_fe_normalizes_to_zero_var(secp256k1_fe *r);


static void secp256k1_fe_set_int(secp256k1_fe *r, int a);


static void secp256k1_fe_clear(secp256k1_fe *a);


static int secp256k1_fe_is_zero(const secp256k1_fe *a);


static int secp256k1_fe_is_odd(const secp256k1_fe *a);


static int secp256k1_fe_equal(const secp256k1_fe *a, const secp256k1_fe *b);


static int secp256k1_fe_equal_var(const secp256k1_fe *a, const secp256k1_fe *b);


static int secp256k1_fe_cmp_var(const secp256k1_fe *a, const secp256k1_fe *b);


static int secp256k1_fe_set_b32(secp256k1_fe *r, const unsigned char *a);


static void secp256k1_fe_get_b32(unsigned char *r, const secp256k1_fe *a);


static void secp256k1_fe_negate(secp256k1_fe *r, const secp256k1_fe *a, int m);


static void secp256k1_fe_mul_int(secp256k1_fe *r, int a);


static void secp256k1_fe_add(secp256k1_fe *r, const secp256k1_fe *a);


static void secp256k1_fe_mul(secp256k1_fe *r, const secp256k1_fe *a, const secp256k1_fe * SECP256K1_RESTRICT b);


static void secp256k1_fe_sqr(secp256k1_fe *r, const secp256k1_fe *a);


static int secp256k1_fe_sqrt(secp256k1_fe *r, const secp256k1_fe *a);


static int secp256k1_fe_is_quad_var(const secp256k1_fe *a);


static void secp256k1_fe_inv(secp256k1_fe *r, const secp256k1_fe *a);


static void secp256k1_fe_inv_var(secp256k1_fe *r, const secp256k1_fe *a);


static void secp256k1_fe_inv_all_var(secp256k1_fe *r, const secp256k1_fe *a, size_t len);


static void secp256k1_fe_to_storage(secp256k1_fe_storage *r, const secp256k1_fe *a);


static void secp256k1_fe_from_storage(secp256k1_fe *r, const secp256k1_fe_storage *a);


static void secp256k1_fe_storage_cmov(secp256k1_fe_storage *r, const secp256k1_fe_storage *a, int flag);


static void secp256k1_fe_cmov(secp256k1_fe *r, const secp256k1_fe *a, int flag);

#endif








