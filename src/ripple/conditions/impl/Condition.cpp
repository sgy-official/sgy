

#include <ripple/basics/contract.h>
#include <ripple/conditions/Condition.h>
#include <ripple/conditions/Fulfillment.h>
#include <ripple/conditions/impl/PreimageSha256.h>
#include <ripple/conditions/impl/utils.h>
#include <boost/regex.hpp>
#include <boost/optional.hpp>
#include <vector>
#include <iostream>

namespace ripple {
namespace cryptoconditions {

namespace detail {

constexpr std::size_t fingerprintSize = 32;

std::unique_ptr<Condition>
loadSimpleSha256(Type type, Slice s, std::error_code& ec)
{
    using namespace der;

    auto p = parsePreamble(s, ec);

    if (ec)
        return {};

    if (!isPrimitive(p) || !isContextSpecific(p))
    {
        ec = error::incorrect_encoding;
        return {};
    }

    if (p.tag != 0)
    {
        ec = error::unexpected_tag;
        return {};
    }

    if (p.length != fingerprintSize)
    {
        ec = error::fingerprint_size;
        return {};
    }

    Buffer b = parseOctetString(s, p.length, ec);

    if (ec)
        return {};

    p = parsePreamble(s, ec);

    if (ec)
        return {};

    if (!isPrimitive(p) || !isContextSpecific(p))
    {
        ec = error::malformed_encoding;
        return{};
    }

    if (p.tag != 1)
    {
        ec = error::unexpected_tag;
        return {};
    }

    auto cost = parseInteger<std::uint32_t>(s, p.length, ec);

    if (ec)
        return {};

    if (!s.empty())
    {
        ec = error::trailing_garbage;
        return {};
    }

    switch (type)
    {
    case Type::preimageSha256:
        if (cost > PreimageSha256::maxPreimageLength)
        {
            ec = error::preimage_too_long;
            return {};
        }
        break;

    default:
        break;
    }

    return std::make_unique<Condition>(type, cost, std::move(b));
}

}

std::unique_ptr<Condition>
Condition::deserialize(Slice s, std::error_code& ec)
{
    if (s.empty())
    {
        ec = error::buffer_empty;
        return {};
    }

    using namespace der;

    auto const p = parsePreamble(s, ec);
    if (ec)
        return {};

    if (!isConstructed(p) || !isContextSpecific(p))
    {
        ec = error::malformed_encoding;
        return {};
    }

    if (p.length > s.size())
    {
        ec = error::buffer_underfull;
        return {};
    }

    if (s.size() > maxSerializedCondition)
    {
        ec = error::large_size;
        return {};
    }

    std::unique_ptr<Condition> c;

    switch (p.tag)
    {
    case 0: 
        c = detail::loadSimpleSha256(
            Type::preimageSha256,
            Slice(s.data(), p.length), ec);
        if (!ec)
            s += p.length;
        break;

    case 1: 
        ec = error::unsupported_type;
        return {};

    case 2: 
        ec = error::unsupported_type;
        return {};

    case 3: 
        ec = error::unsupported_type;
        return {};

    case 4: 
        ec = error::unsupported_type;
        return {};

    default:
        ec = error::unknown_type;
        return {};
    }

    if (!s.empty())
    {
        ec = error::trailing_garbage;
        return {};
    }

    return c;
}

}
}
























