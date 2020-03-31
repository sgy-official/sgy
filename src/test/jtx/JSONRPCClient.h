

#ifndef RIPPLE_TEST_HTTPCLIENT_H_INCLUDED
#define RIPPLE_TEST_HTTPCLIENT_H_INCLUDED

#include <test/jtx/AbstractClient.h>
#include <ripple/core/Config.h>
#include <memory>

namespace ripple {
namespace test {


std::unique_ptr<AbstractClient>
makeJSONRPCClient(Config const& cfg, unsigned rpc_version = 2);

} 
} 

#endif








