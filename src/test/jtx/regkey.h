

#ifndef RIPPLE_TEST_JTX_REGKEY_H_INCLUDED
#define RIPPLE_TEST_JTX_REGKEY_H_INCLUDED

#include <test/jtx/Account.h>
#include <test/jtx/tags.h>
#include <ripple/json/json_value.h>

namespace ripple {
namespace test {
namespace jtx {


Json::Value
regkey (Account const& account,
    disabled_t);


Json::Value
regkey (Account const& account,
    Account const& signer);

} 
} 
} 

#endif








