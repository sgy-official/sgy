

#ifndef RIPPLE_RPC_CONTEXT_H_INCLUDED
#define RIPPLE_RPC_CONTEXT_H_INCLUDED

#include <ripple/core/Config.h>
#include <ripple/core/JobQueue.h>
#include <ripple/net/InfoSub.h>
#include <ripple/rpc/Role.h>

#include <ripple/beast/utility/Journal.h>

namespace ripple {

class Application;
class NetworkOPs;
class LedgerMaster;

namespace RPC {


struct Context
{
    
    struct Headers
    {
        boost::string_view user;
        boost::string_view forwardedFor;
    };

    beast::Journal j;
    Json::Value params;
    Application& app;
    Resource::Charge& loadType;
    NetworkOPs& netOps;
    LedgerMaster& ledgerMaster;
    Resource::Consumer& consumer;
    Role role;
    std::shared_ptr<JobQueue::Coro> coro;
    InfoSub::pointer infoSub;
    Headers headers;
};

} 
} 

#endif








