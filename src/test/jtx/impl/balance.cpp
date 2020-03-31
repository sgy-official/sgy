

#include <test/jtx/balance.h>

namespace ripple {
namespace test {
namespace jtx {

void
balance::operator()(Env& env) const
{
    if (isXRP(value_.issue()))
    {
        auto const sle = env.le(account_);
        if (none_)
        {
            env.test.expect(! sle);
        }
        else if (env.test.expect(sle))
        {
            env.test.expect(sle->getFieldAmount(
                sfBalance) == value_);
        }
    }
    else
    {
        auto const sle = env.le(keylet::line(
            account_.id(), value_.issue()));
        if (none_)
        {
            env.test.expect(! sle);
        }
        else if (env.test.expect(sle))
        {
            auto amount =
                sle->getFieldAmount(sfBalance);
            amount.setIssuer(
                value_.issue().account);
            if (account_.id() >
                    value_.issue().account)
                amount.negate();
            env.test.expect(amount == value_);
        }
    }
}

} 
} 
} 
























