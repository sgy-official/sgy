

#ifndef RIPPLE_TEST_JTX_RATE_H_INCLUDED
#define RIPPLE_TEST_JTX_RATE_H_INCLUDED

#include <test/jtx/Account.h>
#include <ripple/json/json_value.h>

namespace ripple {
namespace test {
namespace jtx {


Json::Value
rate (Account const& account,
    double multiplier);

} 
} 
} 

#endif








