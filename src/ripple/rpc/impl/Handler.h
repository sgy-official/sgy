

#ifndef RIPPLE_RPC_HANDLER_H_INCLUDED
#define RIPPLE_RPC_HANDLER_H_INCLUDED

#include <ripple/core/Config.h>
#include <ripple/rpc/RPCHandler.h>
#include <ripple/rpc/Status.h>
#include <vector>

namespace Json {
class Object;
}

namespace ripple {
namespace RPC {

enum Condition {
    NO_CONDITION     = 0,
    NEEDS_NETWORK_CONNECTION  = 1,
    NEEDS_CURRENT_LEDGER  = 2 + NEEDS_NETWORK_CONNECTION,
    NEEDS_CLOSED_LEDGER   = 4 + NEEDS_NETWORK_CONNECTION,
};

struct Handler
{
    template <class JsonValue>
    using Method = std::function <Status (Context&, JsonValue&)>;

    const char* name_;
    Method<Json::Value> valueMethod_;
    Role role_;
    RPC::Condition condition_;
};

Handler const* getHandler (std::string const&);


template <class Value>
Json::Value makeObjectValue (
    Value const& value, Json::StaticString const& field = jss::message)
{
    Json::Value result (Json::objectValue);
    result[field] = value;
    return result;
}


std::vector<char const*> getHandlerNames();

} 
} 

#endif








