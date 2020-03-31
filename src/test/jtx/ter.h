

#ifndef RIPPLE_TEST_JTX_TER_H_INCLUDED
#define RIPPLE_TEST_JTX_TER_H_INCLUDED

#include <test/jtx/Env.h>
#include <tuple>

namespace ripple {
namespace test {
namespace jtx {


class ter
{
private:
    boost::optional<TER> v_;

public:
    explicit
    ter (decltype(std::ignore))
    {
    }

    explicit
    ter (TER v)
        : v_(v)
    {
    }

    void
    operator()(Env&, JTx& jt) const
    {
        jt.ter = v_;
    }
};

} 
} 
} 

#endif








