

#ifndef _SECP256K1_FIELD_INNER5X52_IMPL_H_
#define _SECP256K1_FIELD_INNER5X52_IMPL_H_

#include <stdint.h>

#ifdef VERIFY
#define VERIFY_BITS(x, n) VERIFY_CHECK(((x) >> (n)) == 0)
#else
#define VERIFY_BITS(x, n) do { } while(0)
#endif

SECP256K1_INLINE static void secp256k1_fe_mul_inner(uint64_t *r, const uint64_t *a, const uint64_t * SECP256K1_RESTRICT b) {
    uint128_t c, d;
    uint64_t t3, t4, tx, u0;
    uint64_t a0 = a[0], a1 = a[1], a2 = a[2], a3 = a[3], a4 = a[4];
    const uint64_t M = 0xFFFFFFFFFFFFFULL, R = 0x1000003D10ULL;

    VERIFY_BITS(a[0], 56);
    VERIFY_BITS(a[1], 56);
    VERIFY_BITS(a[2], 56);
    VERIFY_BITS(a[3], 56);
    VERIFY_BITS(a[4], 52);
    VERIFY_BITS(b[0], 56);
    VERIFY_BITS(b[1], 56);
    VERIFY_BITS(b[2], 56);
    VERIFY_BITS(b[3], 56);
    VERIFY_BITS(b[4], 52);
    VERIFY_CHECK(r != b);

    

    d  = (uint128_t)a0 * b[3]
       + (uint128_t)a1 * b[2]
       + (uint128_t)a2 * b[1]
       + (uint128_t)a3 * b[0];
    VERIFY_BITS(d, 114);
    
    c  = (uint128_t)a4 * b[4];
    VERIFY_BITS(c, 112);
    
    d += (c & M) * R; c >>= 52;
    VERIFY_BITS(d, 115);
    VERIFY_BITS(c, 60);
    
    t3 = d & M; d >>= 52;
    VERIFY_BITS(t3, 52);
    VERIFY_BITS(d, 63);
    

    d += (uint128_t)a0 * b[4]
       + (uint128_t)a1 * b[3]
       + (uint128_t)a2 * b[2]
       + (uint128_t)a3 * b[1]
       + (uint128_t)a4 * b[0];
    VERIFY_BITS(d, 115);
    
    d += c * R;
    VERIFY_BITS(d, 116);
    
    t4 = d & M; d >>= 52;
    VERIFY_BITS(t4, 52);
    VERIFY_BITS(d, 64);
    
    tx = (t4 >> 48); t4 &= (M >> 4);
    VERIFY_BITS(tx, 4);
    VERIFY_BITS(t4, 48);
    

    c  = (uint128_t)a0 * b[0];
    VERIFY_BITS(c, 112);
    
    d += (uint128_t)a1 * b[4]
       + (uint128_t)a2 * b[3]
       + (uint128_t)a3 * b[2]
       + (uint128_t)a4 * b[1];
    VERIFY_BITS(d, 115);
    
    u0 = d & M; d >>= 52;
    VERIFY_BITS(u0, 52);
    VERIFY_BITS(d, 63);
    
    
    u0 = (u0 << 4) | tx;
    VERIFY_BITS(u0, 56);
    
    c += (uint128_t)u0 * (R >> 4);
    VERIFY_BITS(c, 115);
    
    r[0] = c & M; c >>= 52;
    VERIFY_BITS(r[0], 52);
    VERIFY_BITS(c, 61);
    

    c += (uint128_t)a0 * b[1]
       + (uint128_t)a1 * b[0];
    VERIFY_BITS(c, 114);
    
    d += (uint128_t)a2 * b[4]
       + (uint128_t)a3 * b[3]
       + (uint128_t)a4 * b[2];
    VERIFY_BITS(d, 114);
    
    c += (d & M) * R; d >>= 52;
    VERIFY_BITS(c, 115);
    VERIFY_BITS(d, 62);
    
    r[1] = c & M; c >>= 52;
    VERIFY_BITS(r[1], 52);
    VERIFY_BITS(c, 63);
    

    c += (uint128_t)a0 * b[2]
       + (uint128_t)a1 * b[1]
       + (uint128_t)a2 * b[0];
    VERIFY_BITS(c, 114);
    
    d += (uint128_t)a3 * b[4]
       + (uint128_t)a4 * b[3];
    VERIFY_BITS(d, 114);
    
    c += (d & M) * R; d >>= 52;
    VERIFY_BITS(c, 115);
    VERIFY_BITS(d, 62);
    

    
    r[2] = c & M; c >>= 52;
    VERIFY_BITS(r[2], 52);
    VERIFY_BITS(c, 63);
    
    c   += d * R + t3;
    VERIFY_BITS(c, 100);
    
    r[3] = c & M; c >>= 52;
    VERIFY_BITS(r[3], 52);
    VERIFY_BITS(c, 48);
    
    c   += t4;
    VERIFY_BITS(c, 49);
    
    r[4] = c;
    VERIFY_BITS(r[4], 49);
    
}

SECP256K1_INLINE static void secp256k1_fe_sqr_inner(uint64_t *r, const uint64_t *a) {
    uint128_t c, d;
    uint64_t a0 = a[0], a1 = a[1], a2 = a[2], a3 = a[3], a4 = a[4];
    int64_t t3, t4, tx, u0;
    const uint64_t M = 0xFFFFFFFFFFFFFULL, R = 0x1000003D10ULL;

    VERIFY_BITS(a[0], 56);
    VERIFY_BITS(a[1], 56);
    VERIFY_BITS(a[2], 56);
    VERIFY_BITS(a[3], 56);
    VERIFY_BITS(a[4], 52);

    

    d  = (uint128_t)(a0*2) * a3
       + (uint128_t)(a1*2) * a2;
    VERIFY_BITS(d, 114);
    
    c  = (uint128_t)a4 * a4;
    VERIFY_BITS(c, 112);
    
    d += (c & M) * R; c >>= 52;
    VERIFY_BITS(d, 115);
    VERIFY_BITS(c, 60);
    
    t3 = d & M; d >>= 52;
    VERIFY_BITS(t3, 52);
    VERIFY_BITS(d, 63);
    

    a4 *= 2;
    d += (uint128_t)a0 * a4
       + (uint128_t)(a1*2) * a3
       + (uint128_t)a2 * a2;
    VERIFY_BITS(d, 115);
    
    d += c * R;
    VERIFY_BITS(d, 116);
    
    t4 = d & M; d >>= 52;
    VERIFY_BITS(t4, 52);
    VERIFY_BITS(d, 64);
    
    tx = (t4 >> 48); t4 &= (M >> 4);
    VERIFY_BITS(tx, 4);
    VERIFY_BITS(t4, 48);
    

    c  = (uint128_t)a0 * a0;
    VERIFY_BITS(c, 112);
    
    d += (uint128_t)a1 * a4
       + (uint128_t)(a2*2) * a3;
    VERIFY_BITS(d, 114);
    
    u0 = d & M; d >>= 52;
    VERIFY_BITS(u0, 52);
    VERIFY_BITS(d, 62);
    
    
    u0 = (u0 << 4) | tx;
    VERIFY_BITS(u0, 56);
    
    c += (uint128_t)u0 * (R >> 4);
    VERIFY_BITS(c, 113);
    
    r[0] = c & M; c >>= 52;
    VERIFY_BITS(r[0], 52);
    VERIFY_BITS(c, 61);
    

    a0 *= 2;
    c += (uint128_t)a0 * a1;
    VERIFY_BITS(c, 114);
    
    d += (uint128_t)a2 * a4
       + (uint128_t)a3 * a3;
    VERIFY_BITS(d, 114);
    
    c += (d & M) * R; d >>= 52;
    VERIFY_BITS(c, 115);
    VERIFY_BITS(d, 62);
    
    r[1] = c & M; c >>= 52;
    VERIFY_BITS(r[1], 52);
    VERIFY_BITS(c, 63);
    

    c += (uint128_t)a0 * a2
       + (uint128_t)a1 * a1;
    VERIFY_BITS(c, 114);
    
    d += (uint128_t)a3 * a4;
    VERIFY_BITS(d, 114);
    
    c += (d & M) * R; d >>= 52;
    VERIFY_BITS(c, 115);
    VERIFY_BITS(d, 62);
    
    r[2] = c & M; c >>= 52;
    VERIFY_BITS(r[2], 52);
    VERIFY_BITS(c, 63);
    

    c   += d * R + t3;
    VERIFY_BITS(c, 100);
    
    r[3] = c & M; c >>= 52;
    VERIFY_BITS(r[3], 52);
    VERIFY_BITS(c, 48);
    
    c   += t4;
    VERIFY_BITS(c, 49);
    
    r[4] = c;
    VERIFY_BITS(r[4], 49);
    
}

#endif








