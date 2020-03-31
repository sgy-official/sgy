

#include <test/jtx/pay.h>
#include <ripple/protocol/jss.h>
#include <ripple/protocol/TxFlags.h>

namespace ripple {
namespace test {
namespace jtx {

Json::Value
pay (Account const& account,
    Account const& to,
        AnyAmount amount)
{
    amount.to(to);
    Json::Value jv;
    jv[jss::Account] = account.human();
    jv[jss::Amount] = amount.value.getJson(JsonOptions::none);
    jv[jss::Destination] = to.human();
    jv[jss::TransactionType] = jss::Payment;
    jv[jss::Flags] = tfUniversal;
    return jv;
}

} 
} 
} 
























