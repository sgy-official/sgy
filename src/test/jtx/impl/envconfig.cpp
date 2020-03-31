

#include <test/jtx/envconfig.h>
#include <test/jtx/Env.h>
#include <ripple/core/ConfigSections.h>

namespace ripple {
namespace test {

int port_base = 8000;
void incPorts()
{
    port_base += 3;
}

std::atomic<bool> envUseIPv4 {false};

void
setupConfigForUnitTests (Config& cfg)
{
    std::string const port_peer = to_string(port_base);
    std::string port_rpc = to_string(port_base + 1);
    std::string port_ws = to_string(port_base + 2);

    cfg.overwrite (ConfigSection::nodeDatabase (), "type", "memory");
    cfg.overwrite (ConfigSection::nodeDatabase (), "path", "main");
    cfg.deprecatedClearSection (ConfigSection::importNodeDatabase ());
    cfg.legacy("database_path", "");
    cfg.setupControl(true, true, true);
    cfg["server"].append("port_peer");
    cfg["port_peer"].set("ip", getEnvLocalhostAddr());
    cfg["port_peer"].set("port", port_peer);
    cfg["port_peer"].set("protocol", "peer");

    cfg["server"].append("port_rpc");
    cfg["port_rpc"].set("ip", getEnvLocalhostAddr());
    cfg["port_rpc"].set("admin", getEnvLocalhostAddr());
    cfg["port_rpc"].set("port", port_rpc);
    cfg["port_rpc"].set("protocol", "http,ws2");

    cfg["server"].append("port_ws");
    cfg["port_ws"].set("ip", getEnvLocalhostAddr());
    cfg["port_ws"].set("admin", getEnvLocalhostAddr());
    cfg["port_ws"].set("port", port_ws);
    cfg["port_ws"].set("protocol", "ws");
}

namespace jtx {

std::unique_ptr<Config>
no_admin(std::unique_ptr<Config> cfg)
{
    (*cfg)["port_rpc"].set("admin","");
    (*cfg)["port_ws"].set("admin","");
    return cfg;
}

std::unique_ptr<Config>
secure_gateway(std::unique_ptr<Config> cfg)
{
    (*cfg)["port_rpc"].set("admin", "");
    (*cfg)["port_ws"].set("admin","");
    (*cfg)["port_rpc"].set("secure_gateway", getEnvLocalhostAddr());
    return cfg;
}

auto constexpr defaultseed = "shUwVw52ofnCUX5m7kPTKzJdr4HEH";

std::unique_ptr<Config>
validator(std::unique_ptr<Config> cfg, std::string const& seed)
{
    cfg->section(SECTION_VALIDATION_SEED).append(
        std::vector<std::string>{seed.empty() ? defaultseed : seed});
    return cfg;
}

std::unique_ptr<Config>
port_increment(std::unique_ptr<Config> cfg, int increment)
{
    for (auto const sectionName : {"port_peer", "port_rpc", "port_ws"})
    {
        Section& s = (*cfg)[sectionName];
        auto const port = s.get<std::int32_t>("port");
        if (port)
        {
            s.set ("port", std::to_string(*port + increment));
        }
    }
    return cfg;
}

} 
} 
} 
























