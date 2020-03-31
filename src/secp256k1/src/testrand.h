

#ifndef _SECP256K1_TESTRAND_H_
#define _SECP256K1_TESTRAND_H_

#if defined HAVE_CONFIG_H
#include "libsecp256k1-config.h"
#endif




SECP256K1_INLINE static void secp256k1_rand_seed(const unsigned char *seed16);


static uint32_t secp256k1_rand32(void);


static uint32_t secp256k1_rand_bits(int bits);


static uint32_t secp256k1_rand_int(uint32_t range);


static void secp256k1_rand256(unsigned char *b32);


static void secp256k1_rand256_test(unsigned char *b32);


static void secp256k1_rand_bytes_test(unsigned char *bytes, size_t len);

#endif








