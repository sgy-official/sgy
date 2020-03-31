

#ifndef RIPPLE_JSON_OUTPUT_H_INCLUDED
#define RIPPLE_JSON_OUTPUT_H_INCLUDED

#include <boost/beast/core/string.hpp>
#include <functional>

namespace Json {

class Value;

using Output = std::function <void (boost::beast::string_view const&)>;

inline
Output stringOutput (std::string& s)
{
    return [&](boost::beast::string_view const& b) { s.append (b.data(), b.size()); };
}


void outputJson (Json::Value const&, Output const&);


std::string jsonAsString (Json::Value const&);

} 

#endif








