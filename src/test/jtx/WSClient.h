

#ifndef RIPPLE_TEST_WSCLIENT_H_INCLUDED
#define RIPPLE_TEST_WSCLIENT_H_INCLUDED

#include <test/jtx/AbstractClient.h>
#include <ripple/core/Config.h>
#include <boost/optional.hpp>
#include <chrono>
#include <memory>

namespace ripple {
namespace test {

class WSClient : public AbstractClient
{
public:
    
    virtual
    boost::optional<Json::Value>
    getMsg(std::chrono::milliseconds const& timeout =
        std::chrono::milliseconds{0}) = 0;

    
    virtual
    boost::optional<Json::Value>
    findMsg(std::chrono::milliseconds const& timeout,
        std::function<bool(Json::Value const&)> pred) = 0;
};


std::unique_ptr<WSClient>
makeWSClient(Config const& cfg, bool v2 = true, unsigned rpc_version = 2,
    std::unordered_map<std::string, std::string> const& headers = {});

} 
} 

#endif








