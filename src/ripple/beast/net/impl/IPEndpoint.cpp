

#include <ripple/beast/net/IPEndpoint.h>
#include <boost/algorithm/string.hpp>

namespace beast {
namespace IP {

Endpoint::Endpoint ()
    : m_port (0)
{
}

Endpoint::Endpoint (Address const& addr, Port port)
    : m_addr (addr)
    , m_port (port)
{
}

std::pair <Endpoint, bool> Endpoint::from_string_checked (std::string const& s)
{
    std::stringstream is (boost::trim_copy(s));
    Endpoint endpoint;
    is >> endpoint;
    if (! is.fail() && is.rdbuf()->in_avail() == 0)
        return std::make_pair (endpoint, true);
    return std::make_pair (Endpoint {}, false);
}

Endpoint Endpoint::from_string (std::string const& s)
{
    std::pair <Endpoint, bool> const result (
        from_string_checked (s));
    if (result.second)
        return result.first;
    return Endpoint {};
}

std::string Endpoint::to_string () const
{
    std::string s;
    s.reserve(
        (address().is_v6() ? INET6_ADDRSTRLEN-1 : 15) +
        (port() == 0 ? 0 : 6 + (address().is_v6() ? 2 : 0)));

    if (port() != 0 && address().is_v6())
        s += '[';
    s += address ().to_string();
    if (port())
    {
        if (address().is_v6())
            s += ']';
        s += ":" + std::to_string (port());
    }

    return s;
}

bool operator== (Endpoint const& lhs, Endpoint const& rhs)
{
    return lhs.address() == rhs.address() &&
           lhs.port() == rhs.port();
}

bool operator<  (Endpoint const& lhs, Endpoint const& rhs)
{
    if (lhs.address() < rhs.address())
        return true;
    if (lhs.address() > rhs.address())
        return false;
    return lhs.port() < rhs.port();
}


std::istream& operator>> (std::istream& is, Endpoint& endpoint)
{
    std::string addrStr;
    addrStr.reserve(INET6_ADDRSTRLEN);
    char i {0};
    char readTo {0};
    is.get(i);
    if (i == '[') 
        readTo = ']';
    else
        addrStr+=i;

    while (is && is.rdbuf()->in_avail() > 0 && is.get(i))
    {
        if (isspace(static_cast<unsigned char>(i)) || (readTo && i == readTo))
            break;

        if ((i == '.') ||
            (i >= '0' && i <= ':') ||
            (i >= 'a' && i <= 'f') ||
            (i >= 'A' && i <= 'F'))
        {
            addrStr+=i;

            if ( addrStr.size() == INET6_ADDRSTRLEN ||
                (readTo && readTo == ':' && addrStr.size() > 15))
            {
                is.setstate (std::ios_base::failbit);
                return is;
            }

            if (! readTo && (i == '.' || i == ':'))
            {
                readTo = (i == '.') ? ':' : ' ';
            }
        }
        else 
        {
            is.unget();
            is.setstate (std::ios_base::failbit);
            return is;
        }
    }

    if (readTo == ']' && is.rdbuf()->in_avail() > 0)
    {
        is.get(i);
        if (! (isspace(static_cast<unsigned char>(i)) || i == ':'))
        {
            is.unget();
            is.setstate (std::ios_base::failbit);
            return is;
        }
    }

    boost::system::error_code ec;
    auto addr = Address::from_string(addrStr, ec);
    if (ec)
    {
        is.setstate (std::ios_base::failbit);
        return is;
    }

    if (is.rdbuf()->in_avail() > 0)
    {
        Port port;
        is >> port;
        if (is.fail())
            return is;
        endpoint = Endpoint (addr, port);
    }
    else
        endpoint = Endpoint (addr);

    return is;
}

}
}
























