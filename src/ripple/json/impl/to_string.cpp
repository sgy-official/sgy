

#include <ripple/json/json_writer.h>
#include <ripple/json/to_string.h>

namespace Json
{

std::string to_string (Value const& value)
{
    return FastWriter ().write (value);
}

std::string pretty (Value const& value)
{
    return StyledWriter().write (value);
}

} 
























