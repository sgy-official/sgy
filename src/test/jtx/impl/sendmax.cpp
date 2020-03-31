

#include <test/jtx/sendmax.h>
#include <ripple/protocol/jss.h>

namespace ripple {
namespace test {
namespace jtx {

void
sendmax::operator()(Env& env, JTx& jt) const
{
    jt.jv[jss::SendMax] = amount_.getJson(JsonOptions::none);
}

} 
} 
} 
























