

#ifndef RIPPLE_SERVER_JSONRPCUTIL_H_INCLUDED
#define RIPPLE_SERVER_JSONRPCUTIL_H_INCLUDED

#include <ripple/json/json_value.h>
#include <ripple/json/Output.h>

namespace ripple {

void HTTPReply (
    int nStatus, std::string const& strMsg, Json::Output const&, beast::Journal j);

} 

#endif








