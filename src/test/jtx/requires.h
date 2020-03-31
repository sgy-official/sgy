

#ifndef RIPPLE_TEST_JTX_REQUIRES_H_INCLUDED
#define RIPPLE_TEST_JTX_REQUIRES_H_INCLUDED

#include <functional>
#include <vector>

namespace ripple {
namespace test {
namespace jtx {

class Env;

using require_t = std::function<void(Env&)>;
using requires_t = std::vector<require_t>;

} 
} 
} 

#endif








