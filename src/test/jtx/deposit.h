

#ifndef RIPPLE_TEST_JTX_DEPOSIT_H_INCLUDED
#define RIPPLE_TEST_JTX_DEPOSIT_H_INCLUDED

#include <test/jtx/Env.h>
#include <test/jtx/Account.h>

namespace ripple {
namespace test {
namespace jtx {


namespace deposit {


Json::Value
auth (Account const& account, Account const& auth);


Json::Value
unauth (Account const& account, Account const& unauth);

} 

} 

} 
} 

#endif








