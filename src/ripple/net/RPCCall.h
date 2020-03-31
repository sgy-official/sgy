

#ifndef RIPPLE_NET_RPCCALL_H_INCLUDED
#define RIPPLE_NET_RPCCALL_H_INCLUDED

#include <ripple/basics/Log.h>
#include <ripple/core/Config.h>
#include <ripple/json/json_value.h>
#include <boost/asio/io_service.hpp>
#include <functional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ripple {



namespace RPCCall {

int fromCommandLine (
    Config const& config,
    const std::vector<std::string>& vCmd,
    Logs& logs);

void fromNetwork (
    boost::asio::io_service& io_service,
    std::string const& strIp, const std::uint16_t iPort,
    std::string const& strUsername, std::string const& strPassword,
    std::string const& strPath, std::string const& strMethod,
    Json::Value const& jvParams, const bool bSSL, bool quiet,
    Logs& logs,
    std::function<void (Json::Value const& jvInput)> callbackFuncP = std::function<void (Json::Value const& jvInput)> (),
    std::unordered_map<std::string, std::string> headers = {});
}


Json::Value
cmdLineToJSONRPC (std::vector<std::string> const& args, beast::Journal j);


std::pair<int, Json::Value>
rpcClient(std::vector<std::string> const& args,
    Config const& config, Logs& logs,
    std::unordered_map<std::string, std::string> const& headers = {});

} 

#endif








