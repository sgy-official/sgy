

#ifndef _SECP256K1_ECMULT_IMPL_H_
#define _SECP256K1_ECMULT_IMPL_H_

#include <string.h>

#include "group.h"
#include "scalar.h"
#include "ecmult.h"

#if defined(EXHAUSTIVE_TEST_ORDER)

#  if EXHAUSTIVE_TEST_ORDER > 128
#    define WINDOW_A 5
#    define WINDOW_G 8
#  elif EXHAUSTIVE_TEST_ORDER > 8
#    define WINDOW_A 4
#    define WINDOW_G 4
#  else
#    define WINDOW_A 2
#    define WINDOW_G 2
#  endif
#else

#define WINDOW_A 5

#ifdef USE_ENDOMORPHISM

#define WINDOW_G 15
#else

#define WINDOW_G 16
#endif
#endif


#define ECMULT_TABLE_SIZE(w) (1 << ((w)-2))


static void secp256k1_ecmult_odd_multiples_table(int n, secp256k1_gej *prej, secp256k1_fe *zr, const secp256k1_gej *a) {
    secp256k1_gej d;
    secp256k1_ge a_ge, d_ge;
    int i;

    VERIFY_CHECK(!a->infinity);

    secp256k1_gej_double_var(&d, a, NULL);

    
    d_ge.x = d.x;
    d_ge.y = d.y;
    d_ge.infinity = 0;

    secp256k1_ge_set_gej_zinv(&a_ge, a, &d.z);
    prej[0].x = a_ge.x;
    prej[0].y = a_ge.y;
    prej[0].z = a->z;
    prej[0].infinity = 0;

    zr[0] = d.z;
    for (i = 1; i < n; i++) {
        secp256k1_gej_add_ge_var(&prej[i], &prej[i-1], &d_ge, &zr[i]);
    }

    
    secp256k1_fe_mul(&prej[n-1].z, &prej[n-1].z, &d.z);
}


static void secp256k1_ecmult_odd_multiples_table_globalz_windowa(secp256k1_ge *pre, secp256k1_fe *globalz, const secp256k1_gej *a) {
    secp256k1_gej prej[ECMULT_TABLE_SIZE(WINDOW_A)];
    secp256k1_fe zr[ECMULT_TABLE_SIZE(WINDOW_A)];

    
    secp256k1_ecmult_odd_multiples_table(ECMULT_TABLE_SIZE(WINDOW_A), prej, zr, a);
    
    secp256k1_ge_globalz_set_table_gej(ECMULT_TABLE_SIZE(WINDOW_A), pre, globalz, prej, zr);
}

static void secp256k1_ecmult_odd_multiples_table_storage_var(int n, secp256k1_ge_storage *pre, const secp256k1_gej *a, const secp256k1_callback *cb) {
    secp256k1_gej *prej = (secp256k1_gej*)checked_malloc(cb, sizeof(secp256k1_gej) * n);
    secp256k1_ge *prea = (secp256k1_ge*)checked_malloc(cb, sizeof(secp256k1_ge) * n);
    secp256k1_fe *zr = (secp256k1_fe*)checked_malloc(cb, sizeof(secp256k1_fe) * n);
    int i;

    
    secp256k1_ecmult_odd_multiples_table(n, prej, zr, a);
    
    secp256k1_ge_set_table_gej_var(prea, prej, zr, n);
    
    for (i = 0; i < n; i++) {
        secp256k1_ge_to_storage(&pre[i], &prea[i]);
    }

    free(prea);
    free(prej);
    free(zr);
}


#define ECMULT_TABLE_GET_GE(r,pre,n,w) do { \
    VERIFY_CHECK(((n) & 1) == 1); \
    VERIFY_CHECK((n) >= -((1 << ((w)-1)) - 1)); \
    VERIFY_CHECK((n) <=  ((1 << ((w)-1)) - 1)); \
    if ((n) > 0) { \
        *(r) = (pre)[((n)-1)/2]; \
    } else { \
        secp256k1_ge_neg((r), &(pre)[(-(n)-1)/2]); \
    } \
} while(0)

#define ECMULT_TABLE_GET_GE_STORAGE(r,pre,n,w) do { \
    VERIFY_CHECK(((n) & 1) == 1); \
    VERIFY_CHECK((n) >= -((1 << ((w)-1)) - 1)); \
    VERIFY_CHECK((n) <=  ((1 << ((w)-1)) - 1)); \
    if ((n) > 0) { \
        secp256k1_ge_from_storage((r), &(pre)[((n)-1)/2]); \
    } else { \
        secp256k1_ge_from_storage((r), &(pre)[(-(n)-1)/2]); \
        secp256k1_ge_neg((r), (r)); \
    } \
} while(0)

static void secp256k1_ecmult_context_init(secp256k1_ecmult_context *ctx) {
    ctx->pre_g = NULL;
#ifdef USE_ENDOMORPHISM
    ctx->pre_g_128 = NULL;
#endif
}

static void secp256k1_ecmult_context_build(secp256k1_ecmult_context *ctx, const secp256k1_callback *cb) {
    secp256k1_gej gj;

    if (ctx->pre_g != NULL) {
        return;
    }

    
    secp256k1_gej_set_ge(&gj, &secp256k1_ge_const_g);

    ctx->pre_g = (secp256k1_ge_storage (*)[])checked_malloc(cb, sizeof((*ctx->pre_g)[0]) * ECMULT_TABLE_SIZE(WINDOW_G));

    
    secp256k1_ecmult_odd_multiples_table_storage_var(ECMULT_TABLE_SIZE(WINDOW_G), *ctx->pre_g, &gj, cb);

#ifdef USE_ENDOMORPHISM
    {
        secp256k1_gej g_128j;
        int i;

        ctx->pre_g_128 = (secp256k1_ge_storage (*)[])checked_malloc(cb, sizeof((*ctx->pre_g_128)[0]) * ECMULT_TABLE_SIZE(WINDOW_G));

        
        g_128j = gj;
        for (i = 0; i < 128; i++) {
            secp256k1_gej_double_var(&g_128j, &g_128j, NULL);
        }
        secp256k1_ecmult_odd_multiples_table_storage_var(ECMULT_TABLE_SIZE(WINDOW_G), *ctx->pre_g_128, &g_128j, cb);
    }
#endif
}

static void secp256k1_ecmult_context_clone(secp256k1_ecmult_context *dst,
                                           const secp256k1_ecmult_context *src, const secp256k1_callback *cb) {
    if (src->pre_g == NULL) {
        dst->pre_g = NULL;
    } else {
        size_t size = sizeof((*dst->pre_g)[0]) * ECMULT_TABLE_SIZE(WINDOW_G);
        dst->pre_g = (secp256k1_ge_storage (*)[])checked_malloc(cb, size);
        memcpy(dst->pre_g, src->pre_g, size);
    }
#ifdef USE_ENDOMORPHISM
    if (src->pre_g_128 == NULL) {
        dst->pre_g_128 = NULL;
    } else {
        size_t size = sizeof((*dst->pre_g_128)[0]) * ECMULT_TABLE_SIZE(WINDOW_G);
        dst->pre_g_128 = (secp256k1_ge_storage (*)[])checked_malloc(cb, size);
        memcpy(dst->pre_g_128, src->pre_g_128, size);
    }
#endif
}

static int secp256k1_ecmult_context_is_built(const secp256k1_ecmult_context *ctx) {
    return ctx->pre_g != NULL;
}

static void secp256k1_ecmult_context_clear(secp256k1_ecmult_context *ctx) {
    free(ctx->pre_g);
#ifdef USE_ENDOMORPHISM
    free(ctx->pre_g_128);
#endif
    secp256k1_ecmult_context_init(ctx);
}


static int secp256k1_ecmult_wnaf(int *wnaf, int len, const secp256k1_scalar *a, int w) {
    secp256k1_scalar s = *a;
    int last_set_bit = -1;
    int bit = 0;
    int sign = 1;
    int carry = 0;

    VERIFY_CHECK(wnaf != NULL);
    VERIFY_CHECK(0 <= len && len <= 256);
    VERIFY_CHECK(a != NULL);
    VERIFY_CHECK(2 <= w && w <= 31);

    memset(wnaf, 0, len * sizeof(wnaf[0]));

    if (secp256k1_scalar_get_bits(&s, 255, 1)) {
        secp256k1_scalar_negate(&s, &s);
        sign = -1;
    }

    while (bit < len) {
        int now;
        int word;
        if (secp256k1_scalar_get_bits(&s, bit, 1) == (unsigned int)carry) {
            bit++;
            continue;
        }

        now = w;
        if (now > len - bit) {
            now = len - bit;
        }

        word = secp256k1_scalar_get_bits_var(&s, bit, now) + carry;

        carry = (word >> (w-1)) & 1;
        word -= carry << w;

        wnaf[bit] = sign * word;
        last_set_bit = bit;

        bit += now;
    }
#ifdef VERIFY
    CHECK(carry == 0);
    while (bit < 256) {
        CHECK(secp256k1_scalar_get_bits(&s, bit++, 1) == 0);
    }
#endif
    return last_set_bit + 1;
}

static void secp256k1_ecmult(const secp256k1_ecmult_context *ctx, secp256k1_gej *r, const secp256k1_gej *a, const secp256k1_scalar *na, const secp256k1_scalar *ng) {
    secp256k1_ge pre_a[ECMULT_TABLE_SIZE(WINDOW_A)];
    secp256k1_ge tmpa;
    secp256k1_fe Z;
#ifdef USE_ENDOMORPHISM
    secp256k1_ge pre_a_lam[ECMULT_TABLE_SIZE(WINDOW_A)];
    secp256k1_scalar na_1, na_lam;
    
    secp256k1_scalar ng_1, ng_128;
    int wnaf_na_1[130];
    int wnaf_na_lam[130];
    int bits_na_1;
    int bits_na_lam;
    int wnaf_ng_1[129];
    int bits_ng_1;
    int wnaf_ng_128[129];
    int bits_ng_128;
#else
    int wnaf_na[256];
    int bits_na;
    int wnaf_ng[256];
    int bits_ng;
#endif
    int i;
    int bits;

#ifdef USE_ENDOMORPHISM
    
    secp256k1_scalar_split_lambda(&na_1, &na_lam, na);

    
    bits_na_1   = secp256k1_ecmult_wnaf(wnaf_na_1,   130, &na_1,   WINDOW_A);
    bits_na_lam = secp256k1_ecmult_wnaf(wnaf_na_lam, 130, &na_lam, WINDOW_A);
    VERIFY_CHECK(bits_na_1 <= 130);
    VERIFY_CHECK(bits_na_lam <= 130);
    bits = bits_na_1;
    if (bits_na_lam > bits) {
        bits = bits_na_lam;
    }
#else
    
    bits_na     = secp256k1_ecmult_wnaf(wnaf_na,     256, na,      WINDOW_A);
    bits = bits_na;
#endif

    
    secp256k1_ecmult_odd_multiples_table_globalz_windowa(pre_a, &Z, a);

#ifdef USE_ENDOMORPHISM
    for (i = 0; i < ECMULT_TABLE_SIZE(WINDOW_A); i++) {
        secp256k1_ge_mul_lambda(&pre_a_lam[i], &pre_a[i]);
    }

    
    secp256k1_scalar_split_128(&ng_1, &ng_128, ng);

    
    bits_ng_1   = secp256k1_ecmult_wnaf(wnaf_ng_1,   129, &ng_1,   WINDOW_G);
    bits_ng_128 = secp256k1_ecmult_wnaf(wnaf_ng_128, 129, &ng_128, WINDOW_G);
    if (bits_ng_1 > bits) {
        bits = bits_ng_1;
    }
    if (bits_ng_128 > bits) {
        bits = bits_ng_128;
    }
#else
    bits_ng     = secp256k1_ecmult_wnaf(wnaf_ng,     256, ng,      WINDOW_G);
    if (bits_ng > bits) {
        bits = bits_ng;
    }
#endif

    secp256k1_gej_set_infinity(r);

    for (i = bits - 1; i >= 0; i--) {
        int n;
        secp256k1_gej_double_var(r, r, NULL);
#ifdef USE_ENDOMORPHISM
        if (i < bits_na_1 && (n = wnaf_na_1[i])) {
            ECMULT_TABLE_GET_GE(&tmpa, pre_a, n, WINDOW_A);
            secp256k1_gej_add_ge_var(r, r, &tmpa, NULL);
        }
        if (i < bits_na_lam && (n = wnaf_na_lam[i])) {
            ECMULT_TABLE_GET_GE(&tmpa, pre_a_lam, n, WINDOW_A);
            secp256k1_gej_add_ge_var(r, r, &tmpa, NULL);
        }
        if (i < bits_ng_1 && (n = wnaf_ng_1[i])) {
            ECMULT_TABLE_GET_GE_STORAGE(&tmpa, *ctx->pre_g, n, WINDOW_G);
            secp256k1_gej_add_zinv_var(r, r, &tmpa, &Z);
        }
        if (i < bits_ng_128 && (n = wnaf_ng_128[i])) {
            ECMULT_TABLE_GET_GE_STORAGE(&tmpa, *ctx->pre_g_128, n, WINDOW_G);
            secp256k1_gej_add_zinv_var(r, r, &tmpa, &Z);
        }
#else
        if (i < bits_na && (n = wnaf_na[i])) {
            ECMULT_TABLE_GET_GE(&tmpa, pre_a, n, WINDOW_A);
            secp256k1_gej_add_ge_var(r, r, &tmpa, NULL);
        }
        if (i < bits_ng && (n = wnaf_ng[i])) {
            ECMULT_TABLE_GET_GE_STORAGE(&tmpa, *ctx->pre_g, n, WINDOW_G);
            secp256k1_gej_add_zinv_var(r, r, &tmpa, &Z);
        }
#endif
    }

    if (!r->infinity) {
        secp256k1_fe_mul(&r->z, &r->z, &Z);
    }
}

#endif








