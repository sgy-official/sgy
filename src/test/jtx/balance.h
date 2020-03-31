

#ifndef RIPPLE_TEST_JTX_BALANCE_H_INCLUDED
#define RIPPLE_TEST_JTX_BALANCE_H_INCLUDED

#include <test/jtx/tags.h>
#include <test/jtx/Env.h>

namespace ripple {
namespace test {
namespace jtx {


class balance
{
private:
    bool none_;
    Account account_;
    STAmount value_;

public:
    balance (Account const& account,
            none_t)
        : none_(true)
        , account_(account)
        , value_(XRP)
    {
    }

    balance (Account const& account,
            None const& value)
        : none_(true)
        , account_(account)
        , value_(value.issue)
    {
    }

    balance (Account const& account,
            STAmount const& value)
        : none_(false)
        , account_(account)
        , value_(value)
    {
    }

    void
    operator()(Env&) const;
};

} 
} 
} 

#endif








