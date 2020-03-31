

#ifndef _SECP256K1_NUM_
#define _SECP256K1_NUM_

#ifndef USE_NUM_NONE

#if defined HAVE_CONFIG_H
#include "libsecp256k1-config.h"
#endif

#if defined(USE_NUM_GMP)
#include "num_gmp.h"
#else
#error "Please select num implementation"
#endif


static void secp256k1_num_copy(secp256k1_num *r, const secp256k1_num *a);


static void secp256k1_num_get_bin(unsigned char *r, unsigned int rlen, const secp256k1_num *a);


static void secp256k1_num_set_bin(secp256k1_num *r, const unsigned char *a, unsigned int alen);


static void secp256k1_num_mod_inverse(secp256k1_num *r, const secp256k1_num *a, const secp256k1_num *m);


static int secp256k1_num_jacobi(const secp256k1_num *a, const secp256k1_num *b);


static int secp256k1_num_cmp(const secp256k1_num *a, const secp256k1_num *b);


static int secp256k1_num_eq(const secp256k1_num *a, const secp256k1_num *b);


static void secp256k1_num_add(secp256k1_num *r, const secp256k1_num *a, const secp256k1_num *b);


static void secp256k1_num_sub(secp256k1_num *r, const secp256k1_num *a, const secp256k1_num *b);


static void secp256k1_num_mul(secp256k1_num *r, const secp256k1_num *a, const secp256k1_num *b);


static void secp256k1_num_mod(secp256k1_num *r, const secp256k1_num *m);


static void secp256k1_num_shift(secp256k1_num *r, int bits);


static int secp256k1_num_is_zero(const secp256k1_num *a);


static int secp256k1_num_is_one(const secp256k1_num *a);


static int secp256k1_num_is_neg(const secp256k1_num *a);


static void secp256k1_num_negate(secp256k1_num *r);

#endif

#endif








