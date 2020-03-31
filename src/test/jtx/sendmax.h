

#ifndef RIPPLE_TEST_JTX_SENDMAX_H_INCLUDED
#define RIPPLE_TEST_JTX_SENDMAX_H_INCLUDED

#include <test/jtx/Env.h>
#include <ripple/protocol/STAmount.h>

namespace ripple {
namespace test {
namespace jtx {


class sendmax
{
private:
    STAmount amount_;

public:
    sendmax (STAmount const& amount)
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








