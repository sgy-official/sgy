


#include "ed25519-donna-portable.h"

#if defined(ED25519_SSE2)
#else
	#if defined(HAVE_UINT128) && !defined(ED25519_FORCE_32BIT)
		#define ED25519_64BIT
	#else
		#define ED25519_32BIT
	#endif
#endif

#if !defined(ED25519_NO_INLINE_ASM)
	
	#if defined(ED25519_SSE2)
		#if defined(COMPILER_GCC) && defined(CPU_X86)
			#define ED25519_GCC_32BIT_SSE_CHOOSE
		#elif defined(COMPILER_GCC) && defined(CPU_X86_64)
			#define ED25519_GCC_64BIT_SSE_CHOOSE
		#endif
	#else
		#if defined(CPU_X86_64)
			#if defined(COMPILER_GCC) 
				#if defined(ED25519_64BIT)
					#define ED25519_GCC_64BIT_X86_CHOOSE
				#else
					#define ED25519_GCC_64BIT_32BIT_CHOOSE
				#endif
			#endif
		#endif
	#endif
#endif

#if defined(ED25519_SSE2)
	#include "curve25519-donna-sse2.h"
#elif defined(ED25519_64BIT)
	#include "curve25519-donna-64bit.h"
#else
	#include "curve25519-donna-32bit.h"
#endif

#include "curve25519-donna-helpers.h"


#if defined(HAVE_UINT128) && !defined(ED25519_FORCE_32BIT)
	#include "modm-donna-64bit.h"
#else
	#include "modm-donna-32bit.h"
#endif

typedef unsigned char hash_512bits[64];


static int
ed25519_verify(const unsigned char *x, const unsigned char *y, size_t len) {
	size_t differentbits = 0;
	while (len--)
		differentbits |= (*x++ ^ *y++);
	return (int) (1 & ((differentbits - 1) >> 8));
}




typedef struct ge25519_t {
	bignum25519 x, y, z, t;
} ge25519;

typedef struct ge25519_p1p1_t {
	bignum25519 x, y, z, t;
} ge25519_p1p1;

typedef struct ge25519_niels_t {
	bignum25519 ysubx, xaddy, t2d;
} ge25519_niels;

typedef struct ge25519_pniels_t {
	bignum25519 ysubx, xaddy, z, t2d;
} ge25519_pniels;

#include "ed25519-donna-basepoint-table.h"

#if defined(ED25519_64BIT)
	#include "ed25519-donna-64bit-tables.h"
	#include "ed25519-donna-64bit-x86.h"
#else
	#include "ed25519-donna-32bit-tables.h"
	#include "ed25519-donna-64bit-x86-32bit.h"
#endif


#if defined(ED25519_SSE2)
	#include "ed25519-donna-32bit-sse2.h"
	#include "ed25519-donna-64bit-sse2.h"
	#include "ed25519-donna-impl-sse2.h"
#else
	#include "ed25519-donna-impl-base.h"
#endif









