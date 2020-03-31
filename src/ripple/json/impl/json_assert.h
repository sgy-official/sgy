

#ifndef RIPPLE_JSON_JSON_ASSERT_H_INCLUDED
#define RIPPLE_JSON_JSON_ASSERT_H_INCLUDED

#include "ripple/json/json_errors.h"

#define JSON_ASSERT_UNREACHABLE assert( false )
#define JSON_ASSERT( condition ) assert( condition );  
#define JSON_ASSERT_MESSAGE( condition, message ) if (!( condition )) ripple::Throw<Json::error> ( message );

#endif








