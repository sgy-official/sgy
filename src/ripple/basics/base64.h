




#ifndef RIPPLE_BASICS_BASE64_H_INCLUDED
#define RIPPLE_BASICS_BASE64_H_INCLUDED

#include <string>

namespace ripple {

std::string
base64_encode (std::uint8_t const* data,
    std::size_t len);

inline
std::string
base64_encode(std::string const& s)
{
    return base64_encode (reinterpret_cast <
        std::uint8_t const*> (s.data()), s.size());
}

std::string
base64_decode(std::string const& data);

} 

#endif








