

#ifndef RIPPLE_RPC_REQUEST_H_INCLUDED
#define RIPPLE_RPC_REQUEST_H_INCLUDED

#include <ripple/resource/Charge.h>
#include <ripple/resource/Fees.h>
#include <ripple/json/json_value.h>
#include <beast/utility/Journal.h>

namespace ripple {

class Application;

namespace RPC {

struct Request
{
    explicit Request (
        beast::Journal journal_,
        std::string const& method_,
        Json::Value& params_,
        Application& app_)
        : journal (journal_)
        , method (method_)
        , params (params_)
        , fee (Resource::feeReferenceRPC)
        , app (app_)
    {
    }

    beast::Journal journal;

    std::string method;

    Json::Value params;

    Resource::Charge fee;

    Json::Value result;

    Application& app;

private:
    Request& operator= (Request const&);
};

}
}

#endif








