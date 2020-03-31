

#include <test/jtx/txflags.h>
#include <ripple/protocol/jss.h>

namespace ripple {
namespace test {
namespace jtx {

void
txflags::operator()(Env&, JTx& jt) const
{
    jt[jss::Flags] =
        v_ ;
}

} 
} 
} 
























