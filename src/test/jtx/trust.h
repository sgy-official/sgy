

#ifndef RIPPLE_TEST_JTX_TRUST_H_INCLUDED
#define RIPPLE_TEST_JTX_TRUST_H_INCLUDED

#include <test/jtx/Account.h>
#include <ripple/json/json_value.h>
#include <ripple/protocol/STAmount.h>

namespace ripple {
namespace test {
namespace jtx {


Json::Value
trust (Account const& account,
    STAmount const& amount,
       std::uint32_t flags=0);


Json::Value
trust (Account const& account,
    STAmount const& amount,
    Account const& peer,
    std::uint32_t flags);

} 
} 
} 

#endif








