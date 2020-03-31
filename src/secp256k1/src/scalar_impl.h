

#ifndef _SECP256K1_SCALAR_IMPL_H_
#define _SECP256K1_SCALAR_IMPL_H_

#include "group.h"
#include "scalar.h"

#if defined HAVE_CONFIG_H
#include "libsecp256k1-config.h"
#endif

#if defined(EXHAUSTIVE_TEST_ORDER)
#include "scalar_low_impl.h"
#elif defined(USE_SCALAR_4X64)
#include "scalar_4x64_impl.h"
#elif defined(USE_SCALAR_8X32)
#include "scalar_8x32_impl.h"
#else
#error "Please select scalar implementation"
#endif

#ifndef USE_NUM_NONE
static void secp256k1_scalar_get_num(secp256k1_num *r, const secp256k1_scalar *a) {
    unsigned char c[32];
    secp256k1_scalar_get_b32(c, a);
    secp256k1_num_set_bin(r, c, 32);
}


static void secp256k1_scalar_order_get_num(secp256k1_num *r) {
#if defined(EXHAUSTIVE_TEST_ORDER)
    static const unsigned char order[32] = {
        0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,EXHAUSTIVE_TEST_ORDER
    };
#else
    static const unsigned char order[32] = {
        0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
        0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFE,
        0xBA,0xAE,0xDC,0xE6,0xAF,0x48,0xA0,0x3B,
        0xBF,0xD2,0x5E,0x8C,0xD0,0x36,0x41,0x41
    };
#endif
    secp256k1_num_set_bin(r, order, 32);
}
#endif

static void secp256k1_scalar_inverse(secp256k1_scalar *r, const secp256k1_scalar *x) {
#if defined(EXHAUSTIVE_TEST_ORDER)
    int i;
    *r = 0;
    for (i = 0; i < EXHAUSTIVE_TEST_ORDER; i++)
        if ((i * *x) % EXHAUSTIVE_TEST_ORDER == 1)
            *r = i;
    
    VERIFY_CHECK(*r != 0);
}
#else
    secp256k1_scalar *t;
    int i;
    
    secp256k1_scalar x2, x3, x4, x6, x7, x8, x15, x30, x60, x120, x127;

    secp256k1_scalar_sqr(&x2,  x);
    secp256k1_scalar_mul(&x2, &x2,  x);

    secp256k1_scalar_sqr(&x3, &x2);
    secp256k1_scalar_mul(&x3, &x3,  x);

    secp256k1_scalar_sqr(&x4, &x3);
    secp256k1_scalar_mul(&x4, &x4,  x);

    secp256k1_scalar_sqr(&x6, &x4);
    secp256k1_scalar_sqr(&x6, &x6);
    secp256k1_scalar_mul(&x6, &x6, &x2);

    secp256k1_scalar_sqr(&x7, &x6);
    secp256k1_scalar_mul(&x7, &x7,  x);

    secp256k1_scalar_sqr(&x8, &x7);
    secp256k1_scalar_mul(&x8, &x8,  x);

    secp256k1_scalar_sqr(&x15, &x8);
    for (i = 0; i < 6; i++) {
        secp256k1_scalar_sqr(&x15, &x15);
    }
    secp256k1_scalar_mul(&x15, &x15, &x7);

    secp256k1_scalar_sqr(&x30, &x15);
    for (i = 0; i < 14; i++) {
        secp256k1_scalar_sqr(&x30, &x30);
    }
    secp256k1_scalar_mul(&x30, &x30, &x15);

    secp256k1_scalar_sqr(&x60, &x30);
    for (i = 0; i < 29; i++) {
        secp256k1_scalar_sqr(&x60, &x60);
    }
    secp256k1_scalar_mul(&x60, &x60, &x30);

    secp256k1_scalar_sqr(&x120, &x60);
    for (i = 0; i < 59; i++) {
        secp256k1_scalar_sqr(&x120, &x120);
    }
    secp256k1_scalar_mul(&x120, &x120, &x60);

    secp256k1_scalar_sqr(&x127, &x120);
    for (i = 0; i < 6; i++) {
        secp256k1_scalar_sqr(&x127, &x127);
    }
    secp256k1_scalar_mul(&x127, &x127, &x7);

    
    t = &x127;
    for (i = 0; i < 2; i++) { 
        secp256k1_scalar_sqr(t, t);
    }
    secp256k1_scalar_mul(t, t, x); 
    for (i = 0; i < 4; i++) { 
        secp256k1_scalar_sqr(t, t);
    }
    secp256k1_scalar_mul(t, t, &x3); 
    for (i = 0; i < 2; i++) { 
        secp256k1_scalar_sqr(t, t);
    }
    secp256k1_scalar_mul(t, t, x); 
    for (i = 0; i < 2; i++) { 
        secp256k1_scalar_sqr(t, t);
    }
    secp256k1_scalar_mul(t, t, x); 
    for (i = 0; i < 2; i++) { 
        secp256k1_scalar_sqr(t, t);
    }
    secp256k1_scalar_mul(t, t, x); 
    for (i = 0; i < 4; i++) { 
        secp256k1_scalar_sqr(t, t);
    }
    secp256k1_scalar_mul(t, t, &x3); 
    for (i = 0; i < 3; i++) { 
        secp256k1_scalar_sqr(t, t);
    }
    secp256k1_scalar_mul(t, t, &x2); 
    for (i = 0; i < 4; i++) { 
        secp256k1_scalar_sqr(t, t);
    }
    secp256k1_scalar_mul(t, t, &x3); 
    for (i = 0; i < 5; i++) { 
        secp256k1_scalar_sqr(t, t);
    }
    secp256k1_scalar_mul(t, t, &x3); 
    for (i = 0; i < 4; i++) { 
        secp256k1_scalar_sqr(t, t);
    }
    secp256k1_scalar_mul(t, t, &x2); 
    for (i = 0; i < 2; i++) { 
        secp256k1_scalar_sqr(t, t);
    }
    secp256k1_scalar_mul(t, t, x); 
    for (i = 0; i < 2; i++) { 
        secp256k1_scalar_sqr(t, t);
    }
    secp256k1_scalar_mul(t, t, x); 
    for (i = 0; i < 5; i++) { 
        secp256k1_scalar_sqr(t, t);
    }
    secp256k1_scalar_mul(t, t, &x4); 
    for (i = 0; i < 2; i++) { 
        secp256k1_scalar_sqr(t, t);
    }
    secp256k1_scalar_mul(t, t, x); 
    for (i = 0; i < 3; i++) { 
        secp256k1_scalar_sqr(t, t);
    }
    secp256k1_scalar_mul(t, t, x); 
    for (i = 0; i < 4; i++) { 
        secp256k1_scalar_sqr(t, t);
    }
    secp256k1_scalar_mul(t, t, x); 
    for (i = 0; i < 2; i++) { 
        secp256k1_scalar_sqr(t, t);
    }
    secp256k1_scalar_mul(t, t, x); 
    for (i = 0; i < 10; i++) { 
        secp256k1_scalar_sqr(t, t);
    }
    secp256k1_scalar_mul(t, t, &x3); 
    for (i = 0; i < 4; i++) { 
        secp256k1_scalar_sqr(t, t);
    }
    secp256k1_scalar_mul(t, t, &x3); 
    for (i = 0; i < 9; i++) { 
        secp256k1_scalar_sqr(t, t);
    }
    secp256k1_scalar_mul(t, t, &x8); 
    for (i = 0; i < 2; i++) { 
        secp256k1_scalar_sqr(t, t);
    }
    secp256k1_scalar_mul(t, t, x); 
    for (i = 0; i < 3; i++) { 
        secp256k1_scalar_sqr(t, t);
    }
    secp256k1_scalar_mul(t, t, x); 
    for (i = 0; i < 3; i++) { 
        secp256k1_scalar_sqr(t, t);
    }
    secp256k1_scalar_mul(t, t, x); 
    for (i = 0; i < 5; i++) { 
        secp256k1_scalar_sqr(t, t);
    }
    secp256k1_scalar_mul(t, t, &x4); 
    for (i = 0; i < 2; i++) { 
        secp256k1_scalar_sqr(t, t);
    }
    secp256k1_scalar_mul(t, t, x); 
    for (i = 0; i < 5; i++) { 
        secp256k1_scalar_sqr(t, t);
    }
    secp256k1_scalar_mul(t, t, &x2); 
    for (i = 0; i < 4; i++) { 
        secp256k1_scalar_sqr(t, t);
    }
    secp256k1_scalar_mul(t, t, &x2); 
    for (i = 0; i < 2; i++) { 
        secp256k1_scalar_sqr(t, t);
    }
    secp256k1_scalar_mul(t, t, x); 
    for (i = 0; i < 8; i++) { 
        secp256k1_scalar_sqr(t, t);
    }
    secp256k1_scalar_mul(t, t, &x2); 
    for (i = 0; i < 3; i++) { 
        secp256k1_scalar_sqr(t, t);
    }
    secp256k1_scalar_mul(t, t, &x2); 
    for (i = 0; i < 3; i++) { 
        secp256k1_scalar_sqr(t, t);
    }
    secp256k1_scalar_mul(t, t, x); 
    for (i = 0; i < 6; i++) { 
        secp256k1_scalar_sqr(t, t);
    }
    secp256k1_scalar_mul(t, t, x); 
    for (i = 0; i < 8; i++) { 
        secp256k1_scalar_sqr(t, t);
    }
    secp256k1_scalar_mul(r, t, &x6); 
}

SECP256K1_INLINE static int secp256k1_scalar_is_even(const secp256k1_scalar *a) {
    return !(a->d[0] & 1);
}
#endif

static void secp256k1_scalar_inverse_var(secp256k1_scalar *r, const secp256k1_scalar *x) {
#if defined(USE_SCALAR_INV_BUILTIN)
    secp256k1_scalar_inverse(r, x);
#elif defined(USE_SCALAR_INV_NUM)
    unsigned char b[32];
    secp256k1_num n, m;
    secp256k1_scalar t = *x;
    secp256k1_scalar_get_b32(b, &t);
    secp256k1_num_set_bin(&n, b, 32);
    secp256k1_scalar_order_get_num(&m);
    secp256k1_num_mod_inverse(&n, &n, &m);
    secp256k1_num_get_bin(b, 32, &n);
    secp256k1_scalar_set_b32(r, b, NULL);
    
    secp256k1_scalar_mul(&t, &t, r);
    CHECK(secp256k1_scalar_is_one(&t));
#else
#error "Please select scalar inverse implementation"
#endif
}

#ifdef USE_ENDOMORPHISM
#if defined(EXHAUSTIVE_TEST_ORDER)

static void secp256k1_scalar_split_lambda(secp256k1_scalar *r1, secp256k1_scalar *r2, const secp256k1_scalar *a) {
    *r2 = (*a + 5) % EXHAUSTIVE_TEST_ORDER;
    *r1 = (*a + (EXHAUSTIVE_TEST_ORDER - *r2) * EXHAUSTIVE_TEST_LAMBDA) % EXHAUSTIVE_TEST_ORDER;
}
#else


static void secp256k1_scalar_split_lambda(secp256k1_scalar *r1, secp256k1_scalar *r2, const secp256k1_scalar *a) {
    secp256k1_scalar c1, c2;
    static const secp256k1_scalar minus_lambda = SECP256K1_SCALAR_CONST(
        0xAC9C52B3UL, 0x3FA3CF1FUL, 0x5AD9E3FDUL, 0x77ED9BA4UL,
        0xA880B9FCUL, 0x8EC739C2UL, 0xE0CFC810UL, 0xB51283CFUL
    );
    static const secp256k1_scalar minus_b1 = SECP256K1_SCALAR_CONST(
        0x00000000UL, 0x00000000UL, 0x00000000UL, 0x00000000UL,
        0xE4437ED6UL, 0x010E8828UL, 0x6F547FA9UL, 0x0ABFE4C3UL
    );
    static const secp256k1_scalar minus_b2 = SECP256K1_SCALAR_CONST(
        0xFFFFFFFFUL, 0xFFFFFFFFUL, 0xFFFFFFFFUL, 0xFFFFFFFEUL,
        0x8A280AC5UL, 0x0774346DUL, 0xD765CDA8UL, 0x3DB1562CUL
    );
    static const secp256k1_scalar g1 = SECP256K1_SCALAR_CONST(
        0x00000000UL, 0x00000000UL, 0x00000000UL, 0x00003086UL,
        0xD221A7D4UL, 0x6BCDE86CUL, 0x90E49284UL, 0xEB153DABUL
    );
    static const secp256k1_scalar g2 = SECP256K1_SCALAR_CONST(
        0x00000000UL, 0x00000000UL, 0x00000000UL, 0x0000E443UL,
        0x7ED6010EUL, 0x88286F54UL, 0x7FA90ABFUL, 0xE4C42212UL
    );
    VERIFY_CHECK(r1 != a);
    VERIFY_CHECK(r2 != a);
    
    secp256k1_scalar_mul_shift_var(&c1, a, &g1, 272);
    secp256k1_scalar_mul_shift_var(&c2, a, &g2, 272);
    secp256k1_scalar_mul(&c1, &c1, &minus_b1);
    secp256k1_scalar_mul(&c2, &c2, &minus_b2);
    secp256k1_scalar_add(r2, &c1, &c2);
    secp256k1_scalar_mul(r1, r2, &minus_lambda);
    secp256k1_scalar_add(r1, r1, a);
}
#endif
#endif

#endif








