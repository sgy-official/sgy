

#ifndef RIPPLED_RIPPLE_RPC_HANDLERS_VERSION_H
#define RIPPLED_RIPPLE_RPC_HANDLERS_VERSION_H

#include <ripple/rpc/impl/RPCHelpers.h>

namespace ripple {
namespace RPC {

class VersionHandler
{
public:
    explicit VersionHandler (Context&) {}

    Status check()
    {
        return Status::OK;
    }

    template <class Object>
    void writeResult (Object& obj)
    {
        setVersion (obj);
    }

    static char const* name()
    {
        return "version";
    }

    static Role role()
    {
        return Role::USER;
    }

    static Condition condition()
    {
        return NO_CONDITION;
    }
};

} 
} 

#endif








