

#ifndef RIPPLE_RPC_HANDLERS_GETCOUNTS_H_INCLUDED
#define RIPPLE_RPC_HANDLERS_GETCOUNTS_H_INCLUDED

#include <ripple/json/Object.h>
#include <ripple/app/main/Application.h>

namespace ripple {

Json::Value getCountsJson(Application& app, int minObjectCount);

}

#endif







