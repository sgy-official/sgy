

#ifndef RIPPLE_TEST_JTX_QUALITY_H_INCLUDED
#define RIPPLE_TEST_JTX_QUALITY_H_INCLUDED

#include <test/jtx/Env.h>

namespace ripple {
namespace test {
namespace jtx {


class qualityIn
{
private:
    std::uint32_t qIn_;

public:
    explicit qualityIn (std::uint32_t qIn)
    : qIn_ (qIn)
    {
    }

    void
    operator()(Env&, JTx& jtx) const;
};


class qualityInPercent
{
private:
    std::uint32_t qIn_;

public:
    explicit qualityInPercent (double percent);

    void
    operator()(Env&, JTx& jtx) const;
};


class qualityOut
{
private:
    std::uint32_t qOut_;

public:
    explicit qualityOut (std::uint32_t qOut)
    : qOut_ (qOut)
    {
    }

    void
    operator()(Env&, JTx& jtx) const;
};


class qualityOutPercent
{
private:
    std::uint32_t qOut_;

public:
    explicit qualityOutPercent (double percent);

    void
    operator()(Env&, JTx& jtx) const;
};

} 
} 
} 

#endif








