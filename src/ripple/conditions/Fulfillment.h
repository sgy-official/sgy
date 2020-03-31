

#ifndef RIPPLE_CONDITIONS_FULFILLMENT_H
#define RIPPLE_CONDITIONS_FULFILLMENT_H

#include <ripple/basics/Buffer.h>
#include <ripple/basics/Slice.h>
#include <ripple/conditions/Condition.h>
#include <ripple/conditions/impl/utils.h>
#include <boost/optional.hpp>

namespace ripple {
namespace cryptoconditions {

struct Fulfillment
{
public:
    
    static constexpr std::size_t maxSerializedFulfillment = 256;

    
    static
    std::unique_ptr<Fulfillment>
    deserialize(
        Slice s,
        std::error_code& ec);

public:
    virtual ~Fulfillment() = default;

    
    virtual
    Buffer
    fingerprint() const = 0;

    
    virtual
    Type
    type () const = 0;

    
    virtual
    bool
    validate (Slice data) const = 0;

    
    virtual
    std::uint32_t
    cost() const = 0;

    
    virtual
    Condition
    condition() const = 0;
};

inline
bool
operator== (Fulfillment const& lhs, Fulfillment const& rhs)
{
    return
        lhs.type() == rhs.type() &&
            lhs.cost() == rhs.cost() &&
                lhs.fingerprint() == rhs.fingerprint();
}

inline
bool
operator!= (Fulfillment const& lhs, Fulfillment const& rhs)
{
    return !(lhs == rhs);
}


bool
match (
    Fulfillment const& f,
    Condition const& c);


bool
validate (
    Fulfillment const& f,
    Condition const& c,
    Slice m);


bool
validate (
    Fulfillment const& f,
    Condition const& c);

}
}

#endif








