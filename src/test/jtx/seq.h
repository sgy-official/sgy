

#ifndef RIPPLE_TEST_JTX_SEQ_H_INCLUDED
#define RIPPLE_TEST_JTX_SEQ_H_INCLUDED

#include <test/jtx/Env.h>
#include <test/jtx/tags.h>
#include <boost/optional.hpp>

namespace ripple {
namespace test {
namespace jtx {


struct seq
{
private:
    bool manual_ = true;
    boost::optional<std::uint32_t> num_;

public:
    explicit
    seq (autofill_t)
        : manual_(false)
    {
    }

    explicit
    seq (none_t)
    {
    }

    explicit
    seq (std::uint32_t num)
        : num_(num)
    {
    }

    void
    operator()(Env&, JTx& jt) const;
};

} 
} 
} 

#endif








