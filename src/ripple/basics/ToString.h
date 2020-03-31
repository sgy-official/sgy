

#ifndef RIPPLE_BASICS_TOSTRING_H_INCLUDED
#define RIPPLE_BASICS_TOSTRING_H_INCLUDED

#include <string>
#include <type_traits>

namespace ripple {



template <class T>
typename std::enable_if<std::is_arithmetic<T>::value,
                        std::string>::type
to_string(T t)
{
    return std::to_string(t);
}

inline std::string to_string(bool b)
{
    return b ? "true" : "false";
}

inline std::string to_string(char c)
{
    return std::string(1, c);
}

inline std::string to_string(std::string s)
{
    return s;
}

inline std::string to_string(char const* s)
{
    return s;
}

} 

#endif








