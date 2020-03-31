

#ifndef RIPPLE_TEST_JTX_SIG_H_INCLUDED
#define RIPPLE_TEST_JTX_SIG_H_INCLUDED

#include <test/jtx/Env.h>
#include <boost/optional.hpp>

namespace ripple {
namespace test {
namespace jtx {


class sig
{
private:
    bool manual_ = true;
    boost::optional<Account> account_;

public:
    explicit
    sig (autofill_t)
        : manual_(false)
    {
    }

    explicit
    sig (none_t)
    {
    }

    explicit
    sig (Account const& account)
        : account_(account)
    {
    }

    void
    operator()(Env&, JTx& jt) const;
};

} 
} 
} 

#endif








