
#include <jni.h>
#include "include/secp256k1.h"


#ifndef _Included_org_bitcoin_NativeSecp256k1
#define _Included_org_bitcoin_NativeSecp256k1
#ifdef __cplusplus
extern "C" {
#endif

SECP256K1_API jlong JNICALL Java_org_bitcoin_NativeSecp256k1_secp256k1_1ctx_1clone
  (JNIEnv *, jclass, jlong);


SECP256K1_API jint JNICALL Java_org_bitcoin_NativeSecp256k1_secp256k1_1context_1randomize
  (JNIEnv *, jclass, jobject, jlong);


SECP256K1_API jobjectArray JNICALL Java_org_bitcoin_NativeSecp256k1_secp256k1_1privkey_1tweak_1add
  (JNIEnv *, jclass, jobject, jlong);


SECP256K1_API jobjectArray JNICALL Java_org_bitcoin_NativeSecp256k1_secp256k1_1privkey_1tweak_1mul
  (JNIEnv *, jclass, jobject, jlong);


SECP256K1_API jobjectArray JNICALL Java_org_bitcoin_NativeSecp256k1_secp256k1_1pubkey_1tweak_1add
  (JNIEnv *, jclass, jobject, jlong, jint);


SECP256K1_API jobjectArray JNICALL Java_org_bitcoin_NativeSecp256k1_secp256k1_1pubkey_1tweak_1mul
  (JNIEnv *, jclass, jobject, jlong, jint);


SECP256K1_API void JNICALL Java_org_bitcoin_NativeSecp256k1_secp256k1_1destroy_1context
  (JNIEnv *, jclass, jlong);


SECP256K1_API jint JNICALL Java_org_bitcoin_NativeSecp256k1_secp256k1_1ecdsa_1verify
  (JNIEnv *, jclass, jobject, jlong, jint, jint);


SECP256K1_API jobjectArray JNICALL Java_org_bitcoin_NativeSecp256k1_secp256k1_1ecdsa_1sign
  (JNIEnv *, jclass, jobject, jlong);


SECP256K1_API jint JNICALL Java_org_bitcoin_NativeSecp256k1_secp256k1_1ec_1seckey_1verify
  (JNIEnv *, jclass, jobject, jlong);


SECP256K1_API jobjectArray JNICALL Java_org_bitcoin_NativeSecp256k1_secp256k1_1ec_1pubkey_1create
  (JNIEnv *, jclass, jobject, jlong);


SECP256K1_API jobjectArray JNICALL Java_org_bitcoin_NativeSecp256k1_secp256k1_1ec_1pubkey_1parse
  (JNIEnv *, jclass, jobject, jlong, jint);


SECP256K1_API jobjectArray JNICALL Java_org_bitcoin_NativeSecp256k1_secp256k1_1ecdh
  (JNIEnv* env, jclass classObject, jobject byteBufferObject, jlong ctx_l, jint publen);


#ifdef __cplusplus
}
#endif
#endif








