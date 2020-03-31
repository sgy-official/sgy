

#ifndef RIPPLE_PROTOCOL_BUILDINFO_H_INCLUDED
#define RIPPLE_PROTOCOL_BUILDINFO_H_INCLUDED

#include <cstdint>
#include <string>

namespace ripple {


using ProtocolVersion = std::pair<std::uint16_t, std::uint16_t>;


namespace BuildInfo {


std::string const&
getVersionString();


std::string const&
getFullVersionString();


ProtocolVersion
make_protocol (std::uint32_t version);


ProtocolVersion const&
getCurrentProtocol();


ProtocolVersion const& getMinimumProtocol ();

} 

std::string
to_string (ProtocolVersion const& p);

std::uint32_t
to_packed (ProtocolVersion const& p);

} 

#endif








