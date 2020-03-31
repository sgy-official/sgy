

#include <ripple/basics/safe_cast.h>
#include <ripple/conditions/Condition.h>
#include <ripple/conditions/Fulfillment.h>
#include <ripple/conditions/impl/PreimageSha256.h>
#include <ripple/conditions/impl/utils.h>
#include <boost/regex.hpp>
#include <boost/optional.hpp>
#include <type_traits>
#include <vector>

namespace ripple {
namespace cryptoconditions {

bool
match (
    Fulfillment const& f,
    Condition const& c)
{
    if (f.type() != c.type)
        return false;

    return c == f.condition();
}

bool
validate (
    Fulfillment const& f,
    Condition const& c,
    Slice m)
{
    return match (f, c) && f.validate (m);
}

bool
validate (
    Fulfillment const& f,
    Condition const& c)
{
    return validate (f, c, {});
}

std::unique_ptr<Fulfillment>
Fulfillment::deserialize(
    Slice s,
    std::error_code& ec)
{

    if (s.empty())
    {
        ec = error::buffer_empty;
        return nullptr;
    }

    using namespace der;

    auto const p = parsePreamble(s, ec);
    if (ec)
        return nullptr;

    if (!isConstructed(p) || !isContextSpecific(p))
    {
        ec = error::malformed_encoding;
        return nullptr;
    }

    if (p.length > s.size())
    {
        ec = error::buffer_underfull;
        return {};
    }

    if (p.length < s.size())
    {
        ec = error::buffer_overfull;
        return {};
    }

    if (p.length > maxSerializedFulfillment)
    {
        ec = error::large_size;
        return {};
    }

    std::unique_ptr<Fulfillment> f;

    using TagType = decltype(p.tag);
    switch (p.tag)
    {
    case safe_cast<TagType>(Type::preimageSha256):
        f = PreimageSha256::deserialize(Slice(s.data(), p.length), ec);
        if (ec)
            return {};
        s += p.length;
        break;

    case safe_cast<TagType>(Type::prefixSha256):
        ec = error::unsupported_type;
        return {};
        break;

    case safe_cast<TagType>(Type::thresholdSha256):
        ec = error::unsupported_type;
        return {};
        break;

    case safe_cast<TagType>(Type::rsaSha256):
        ec = error::unsupported_type;
        return {};
        break;

    case safe_cast<TagType>(Type::ed25519Sha256):
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

    return f;
}

}
}
























