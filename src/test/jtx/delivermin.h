

#ifndef RIPPLE_TEST_JTX_DELIVERMIN_H_INCLUDED
#define RIPPLE_TEST_JTX_DELIVERMIN_H_INCLUDED

#include <test/jtx/Env.h>
#include <ripple/protocol/STAmount.h>

namespace ripple {
namespace test {
namespace jtx {


class delivermin
{
private:
    STAmount amount_;

public:
    delivermin (STAmount const& amount)
        : amount_(amount)
    {
    }

    void
    operator()(Env&, JTx& jtx) const;
};

} 
} 
} 

#endif








