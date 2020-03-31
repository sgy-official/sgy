

#ifndef RIPPLE_TEST_JTX_ENVCONFIG_H_INCLUDED
#define RIPPLE_TEST_JTX_ENVCONFIG_H_INCLUDED

#include <ripple/core/Config.h>

namespace ripple {
namespace test {

extern std::atomic<bool> envUseIPv4;

inline
const char *
getEnvLocalhostAddr()
{
    return envUseIPv4 ? "127.0.0.1" : "::1";
}

extern
void
setupConfigForUnitTests (Config& config);

namespace jtx {

inline
std::unique_ptr<Config>
envconfig()
{
    auto p = std::make_unique<Config>();
    setupConfigForUnitTests(*p);
    return p;
}

template <class F, class... Args>
std::unique_ptr<Config>
envconfig(F&& modfunc, Args&&... args)
{
    return modfunc(envconfig(), std::forward<Args>(args)...);
}

std::unique_ptr<Config>
no_admin(std::unique_ptr<Config>);

std::unique_ptr<Config>
secure_gateway(std::unique_ptr<Config>);

std::unique_ptr<Config>
validator(std::unique_ptr<Config>, std::string const&);

std::unique_ptr<Config>
port_increment(std::unique_ptr<Config>, int);

} 
} 
} 

#endif









