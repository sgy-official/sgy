

#ifndef RIPPLE_TEST_JTX_PAY_H_INCLUDED
#define RIPPLE_TEST_JTX_PAY_H_INCLUDED

#include <test/jtx/Account.h>
#include <test/jtx/amount.h>
#include <ripple/json/json_value.h>

namespace ripple {
namespace test {
namespace jtx {


Json::Value
pay (Account const& account,
    Account const& to, AnyAmount amount);

} 
} 
} 

#endif








