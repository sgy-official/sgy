

#ifndef RIPPLE_TEST_JTX_FEE_H_INCLUDED
#define RIPPLE_TEST_JTX_FEE_H_INCLUDED

#include <test/jtx/Env.h>
#include <test/jtx/tags.h>
#include <ripple/protocol/STAmount.h>
#include <ripple/basics/contract.h>
#include <boost/optional.hpp>

namespace ripple {
namespace test {
namespace jtx {


class fee
{
private:
    bool manual_ = true;
    boost::optional<STAmount> amount_;

public:
    explicit
    fee (autofill_t)
        : manual_(false)
    {
    }

    explicit
    fee (none_t)
    {
    }

    explicit
    fee (STAmount const& amount)
        : amount_(amount)
    {
        if (! isXRP(*amount_))
            Throw<std::runtime_error> (
                "fee: not XRP");
    }

    void
    operator()(Env&, JTx& jt) const;
};

} 
} 
} 

#endif








