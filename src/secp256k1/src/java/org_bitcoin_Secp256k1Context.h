
#include <jni.h>
#include "include/secp256k1.h"


#ifndef _Included_org_bitcoin_Secp256k1Context
#define _Included_org_bitcoin_Secp256k1Context
#ifdef __cplusplus
extern "C" {
#endif

SECP256K1_API jlong JNICALL Java_org_bitcoin_Secp256k1Context_secp256k1_1init_1context
  (JNIEnv *, jclass);

#ifdef __cplusplus
}
#endif
#endif








