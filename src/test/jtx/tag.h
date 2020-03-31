

#ifndef RIPPLE_TEST_JTX_TAG_H_INCLUDED
#define RIPPLE_TEST_JTX_TAG_H_INCLUDED

#include <test/jtx/Env.h>

namespace ripple {
namespace test {

namespace jtx {


struct dtag
{
private:
    std::uint32_t value_;

public:
    explicit
    dtag (std::uint32_t value)
        : value_ (value)
    {
    }

    void
    operator()(Env&, JTx& jt) const;
};


struct stag
{
private:
    std::uint32_t value_;

public:
    explicit
    stag (std::uint32_t value)
        : value_ (value)
    {
    }

    void
    operator()(Env&, JTx& jt) const;
};

} 

} 
} 

#endif








