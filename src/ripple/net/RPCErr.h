

#ifndef RIPPLE_NET_RPCERR_H_INCLUDED
#define RIPPLE_NET_RPCERR_H_INCLUDED

#include <ripple/json/json_value.h>

namespace ripple {

bool isRpcError (Json::Value jvResult);
Json::Value rpcError (int iError,
                      Json::Value jvResult = Json::Value (Json::objectValue));

} 

#endif








