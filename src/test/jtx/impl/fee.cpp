

#include <test/jtx/fee.h>
#include <ripple/protocol/jss.h>

namespace ripple {
namespace test {
namespace jtx {

void
fee::operator()(Env&, JTx& jt) const
{
    if (! manual_)
        return;
    jt.fill_fee = false;
    if (amount_)
        jt[jss::Fee] =
            amount_->getJson(JsonOptions::none);
}

} 
} 
} 
























