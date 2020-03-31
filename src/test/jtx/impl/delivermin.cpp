

#include <test/jtx/delivermin.h>
#include <ripple/protocol/jss.h>

namespace ripple {
namespace test {
namespace jtx {

void
delivermin::operator()(Env& env, JTx& jt) const
{
    jt.jv[jss::DeliverMin] = amount_.getJson(JsonOptions::none);
}

} 
} 
} 
























