

#ifndef RIPPLE_TEST_JTX_OWNERS_H_INCLUDED
#define RIPPLE_TEST_JTX_OWNERS_H_INCLUDED

#include <test/jtx/Env.h>
#include <ripple/ledger/View.h>
#include <ripple/protocol/LedgerFormats.h>
#include <ripple/protocol/UintTypes.h>
#include <cstdint>

namespace ripple {
namespace test {
namespace jtx {

namespace detail {

std::uint32_t
owned_count_of (ReadView const& view,
    AccountID const& id,
        LedgerEntryType type);

void
owned_count_helper(Env& env,
    AccountID const& id,
        LedgerEntryType type,
            std::uint32_t value);

} 

template <LedgerEntryType Type>
class owner_count
{
private:
    Account account_;
    std::uint32_t value_;

public:
    owner_count (Account const& account,
            std::uint32_t value)
        : account_(account)
        , value_(value)
    {
    }

    void
    operator()(Env& env) const
    {
        detail::owned_count_helper(
            env, account_.id(), Type, value_);
    }
};


class owners
{
private:
    Account account_;
    std::uint32_t value_;
public:
    owners (Account const& account,
            std::uint32_t value)
        : account_(account)
        , value_(value)
    {
    }

    void
    operator()(Env& env) const;
};


using lines = owner_count<ltRIPPLE_STATE>;


using offers = owner_count<ltOFFER>;

} 
} 
} 

#endif








