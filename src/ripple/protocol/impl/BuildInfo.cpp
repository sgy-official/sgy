

#include <ripple/basics/contract.h>
#include <ripple/beast/core/PlatformConfig.h>
#include <ripple/beast/core/SemanticVersion.h>
#include <ripple/protocol/BuildInfo.h>

namespace ripple {

namespace BuildInfo {

char const* const versionString = "1.3.1"

#if defined(DEBUG) || defined(SANITIZER)
       "+"
#ifdef DEBUG
        "DEBUG"
#ifdef SANITIZER
        "."
#endif
#endif

#ifdef SANITIZER
        BEAST_PP_STR1_(SANITIZER)
#endif
#endif

    ;

ProtocolVersion const&
getCurrentProtocol ()
{
    static ProtocolVersion currentProtocol (
        1,  
        2   
    );

    return currentProtocol;
}

ProtocolVersion const&
getMinimumProtocol ()
{
    static ProtocolVersion minimumProtocol (

        1,  
        2   
    );

    return minimumProtocol;
}


std::string const&
getVersionString ()
{
    static std::string const value = [] {
        std::string const s = versionString;
        beast::SemanticVersion v;
        if (!v.parse (s) || v.print () != s)
            LogicError (s + ": Bad server version string");
        return s;
    }();
    return value;
}

std::string const& getFullVersionString ()
{
    static std::string const value =
        "rippled-" + getVersionString();
    return value;
}

ProtocolVersion
make_protocol (std::uint32_t version)
{
    return ProtocolVersion(
        static_cast<std::uint16_t> ((version >> 16) & 0xffff),
        static_cast<std::uint16_t> (version & 0xffff));
}

}

std::string
to_string (ProtocolVersion const& p)
{
    return std::to_string (p.first) + "." + std::to_string (p.second);
}

std::uint32_t
to_packed (ProtocolVersion const& p)
{
    return (static_cast<std::uint32_t> (p.first) << 16) + p.second;
}

} 
























