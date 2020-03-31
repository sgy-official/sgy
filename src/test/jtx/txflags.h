

#ifndef RIPPLE_TEST_JTX_TXFLAGS_H_INCLUDED
#define RIPPLE_TEST_JTX_TXFLAGS_H_INCLUDED

#include <test/jtx/Env.h>

namespace ripple {
namespace test {
namespace jtx {


class txflags
{
private:
    std::uint32_t v_;

public:
    explicit
    txflags (std::uint32_t v)
        : v_(v)
    {
    }

    void
    operator()(Env&, JTx& jt) const;
};

} 
} 
} 

#endif








