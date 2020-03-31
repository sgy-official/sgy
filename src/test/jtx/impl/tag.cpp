

#include <test/jtx/tag.h>
#include <ripple/protocol/jss.h>

namespace ripple {
namespace test {
namespace jtx {

void
dtag::operator()(Env&, JTx& jt) const
{
    jt.jv["DestinationTag"] = value_;
}

void
stag::operator()(Env&, JTx& jt) const
{
    jt.jv["SourceTag"] = value_;
}

} 
} 
} 
























