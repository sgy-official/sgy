

#ifndef RIPPLE_CONDITIONS_CONDITION_H
#define RIPPLE_CONDITIONS_CONDITION_H

#include <ripple/basics/Buffer.h>
#include <ripple/basics/Slice.h>
#include <ripple/conditions/impl/utils.h>
#include <array>
#include <cstdint>
#include <set>
#include <string>
#include <system_error>
#include <vector>

namespace ripple {
namespace cryptoconditions {

enum class Type
    : std::uint8_t
{
    preimageSha256 = 0,
    prefixSha256 = 1,
    thresholdSha256 = 2,
    rsaSha256 = 3,
    ed25519Sha256 = 4
};

class Condition
{
public:
    
    static constexpr std::size_t maxSerializedCondition = 128;

    
    static
    std::unique_ptr<Condition>
    deserialize(Slice s, std::error_code& ec);

public:
    Type type;

    
    Buffer fingerprint;

    
    std::uint32_t cost;

    
    std::set<Type> subtypes;

    Condition(Type t, std::uint32_t c, Slice fp)
        : type(t)
        , fingerprint(fp)
        , cost(c)
    {
    }

    Condition(Type t, std::uint32_t c, Buffer&& fp)
        : type(t)
        , fingerprint(std::move(fp))
        , cost(c)
    {
    }

    ~Condition() = default;

    Condition(Condition const&) = default;
    Condition(Condition&&) = default;

    Condition() = delete;
};

inline
bool
operator== (Condition const& lhs, Condition const& rhs)
{
    return
        lhs.type == rhs.type &&
            lhs.cost == rhs.cost &&
                lhs.subtypes == rhs.subtypes &&
                    lhs.fingerprint == rhs.fingerprint;
}

inline
bool
operator!= (Condition const& lhs, Condition const& rhs)
{
    return !(lhs == rhs);
}

}

}

#endif








