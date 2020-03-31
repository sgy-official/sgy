

#include <ripple/basics/contract.h>
#include <ripple/basics/Slice.h>
#include <ripple/basics/StringUtilities.h>
#include <ripple/basics/ToString.h>
#include <ripple/beast/core/LexicalCast.h>
#include <boost/algorithm/string.hpp>
#include <ripple/beast/net/IPEndpoint.h>
#include <boost/regex.hpp>
#include <algorithm>
#include <cstdarg>

namespace ripple {

std::pair<Blob, bool> strUnHex (std::string const& strSrc)
{
    Blob out;

    out.reserve ((strSrc.size () + 1) / 2);

    auto iter = strSrc.cbegin ();

    if (strSrc.size () & 1)
    {
        int c = charUnHex (*iter);

        if (c < 0)
            return std::make_pair (Blob (), false);

        out.push_back(c);
        ++iter;
    }

    while (iter != strSrc.cend ())
    {
        int cHigh = charUnHex (*iter);
        ++iter;

        if (cHigh < 0)
            return std::make_pair (Blob (), false);

        int cLow = charUnHex (*iter);
        ++iter;

        if (cLow < 0)
            return std::make_pair (Blob (), false);

        out.push_back (static_cast<unsigned char>((cHigh << 4) | cLow));
    }

    return std::make_pair(std::move(out), true);
}

uint64_t uintFromHex (std::string const& strSrc)
{
    uint64_t uValue (0);

    if (strSrc.size () > 16)
        Throw<std::invalid_argument> ("overlong 64-bit value");

    for (auto c : strSrc)
    {
        int ret = charUnHex (c);

        if (ret == -1)
            Throw<std::invalid_argument> ("invalid hex digit");

        uValue = (uValue << 4) | ret;
    }

    return uValue;
}

bool parseUrl (parsedURL& pUrl, std::string const& strUrl)
{
    static boost::regex reUrl (
        "(?i)\\`\\s*"
        "([[:alpha:]][-+.[:alpha:][:digit:]]*?):"
        "//"
        "(?:([^:@/]*?)(?::([^@/]*?))?@)?"
        "([[:digit:]:]*[[:digit:]]|\\[[^]]+\\]|[^:/?#]*?)"
        "(?::([[:digit:]]+))?"
        "(/.*)?"
        "\\s*?\\'");
    boost::smatch smMatch;

    try {
        if (! boost::regex_match (strUrl, smMatch, reUrl))
            return false;
    } catch (...) {
        return false;
    }

    pUrl.scheme = smMatch[1];
    boost::algorithm::to_lower (pUrl.scheme);
    pUrl.username = smMatch[2];
    pUrl.password = smMatch[3];
    const std::string domain = smMatch[4];
    const auto result {beast::IP::Endpoint::from_string_checked (domain)};
    pUrl.domain = result.second
        ? result.first.address().to_string()
        : domain;
    const std::string port = smMatch[5];
    if (!port.empty())
    {
        pUrl.port = beast::lexicalCast <std::uint16_t> (port);
    }
    pUrl.path = smMatch[6];

    return true;
}

std::string trim_whitespace (std::string str)
{
    boost::trim (str);
    return str;
}

boost::optional<std::uint64_t>
to_uint64(std::string const& s)
{
    std::uint64_t result;
    if (beast::lexicalCastChecked (result, s))
        return result;
    return boost::none;
}

} 
























