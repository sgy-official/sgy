



#ifndef BEAST_HASH_XXHASH_H_INCLUDED
#define BEAST_HASH_XXHASH_H_INCLUDED


#include <stddef.h>   

namespace beast {
namespace detail {


typedef enum { XXH_OK=0, XXH_ERROR } XXH_errorcode;





unsigned int       XXH32 (const void* input, size_t length, unsigned seed);
unsigned long long XXH64 (const void* input, size_t length, unsigned long long seed);






typedef struct { long long ll[ 6]; } XXH32_state_t;
typedef struct { long long ll[11]; } XXH64_state_t;



XXH32_state_t* XXH32_createState(void);
XXH_errorcode  XXH32_freeState(XXH32_state_t* statePtr);

XXH64_state_t* XXH64_createState(void);
XXH_errorcode  XXH64_freeState(XXH64_state_t* statePtr);




XXH_errorcode XXH32_reset  (XXH32_state_t* statePtr, unsigned seed);
XXH_errorcode XXH32_update (XXH32_state_t* statePtr, const void* input, size_t length);
unsigned int  XXH32_digest (const XXH32_state_t* statePtr);

XXH_errorcode      XXH64_reset  (XXH64_state_t* statePtr, unsigned long long seed);
XXH_errorcode      XXH64_update (XXH64_state_t* statePtr, const void* input, size_t length);
unsigned long long XXH64_digest (const XXH64_state_t* statePtr);



} 
} 

#endif








