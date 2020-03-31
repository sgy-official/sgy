

#ifndef RIPPLE_CONDITIONS_PREIMAGE_SHA256_H
#define RIPPLE_CONDITIONS_PREIMAGE_SHA256_H

#include <ripple/basics/Buffer.h>
#include <ripple/basics/Slice.h>
#include <ripple/conditions/Condition.h>
#include <ripple/conditions/Fulfillment.h>
#include <ripple/conditions/impl/error.h>
#include <ripple/protocol/digest.h>
#include <memory>

namespace ripple {
namespace cryptoconditions {

class PreimageSha256 final
    : public Fulfillment
{
public:
    
    static constexpr std::size_t maxPreimageLength = 128;

    
    static
    std::unique_ptr<Fulfillment>
    deserialize(
        Slice s,
        std::error_code& ec)
    {

        using namespace der;

        auto p = parsePreamble(s, ec);
        if (ec)
            return nullptr;

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

        if (s.size() != p.length)
        {
            ec = error::trailing_garbage;
            return {};
        }

        if (s.size() > maxPreimageLength)
        {
            ec = error::preimage_too_long;
            return {};
        }

        auto b = parseOctetString(s, p.length, ec);
        if (ec)
            return {};

        return std::make_unique<PreimageSha256>(std::move(b));
    }

private:
    Buffer payload_;

public:
    PreimageSha256(Buffer&& b) noexcept
        : payload_(std::move(b))
    {
    }

    PreimageSha256(Slice s) noexcept
        : payload_(s)
    {
    }

    Type
    type() const override
    {
        return Type::preimageSha256;
    }

    Buffer
    fingerprint() const override
    {
        sha256_hasher h;
        h(payload_.data(), payload_.size());
        auto const d = static_cast<sha256_hasher::result_type>(h);
        return{ d.data(), d.size() };
    }

    std::uint32_t
    cost() const override
    {
        return static_cast<std::uint32_t>(payload_.size());
    }

    Condition
    condition() const override
    {
        return { type(), cost(), fingerprint() };
    }

    bool
    validate(Slice) const override
    {
        return true;
    }
};

}
}

#endif








