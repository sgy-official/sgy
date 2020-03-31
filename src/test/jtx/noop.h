

#ifndef RIPPLE_TEST_JTX_NOOP_H_INCLUDED
#define RIPPLE_TEST_JTX_NOOP_H_INCLUDED

#include <test/jtx/flags.h>

namespace ripple {
namespace test {
namespace jtx {


inline
Json::Value
noop (Account const& account)
{
    return fset(account, 0);
}

} 
} 
} 

#endif








