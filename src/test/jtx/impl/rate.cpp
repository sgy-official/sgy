

#include <test/jtx/rate.h>
#include <ripple/protocol/jss.h>
#include <ripple/basics/contract.h>
#include <stdexcept>

namespace ripple {
namespace test {
namespace jtx {

Json::Value
rate (Account const& account, double multiplier)
{
    if (multiplier > 4)
        Throw<std::runtime_error> (
            "rate multiplier out of range");
    Json::Value jv;
    jv[jss::Account] = account.human();
    jv[jss::TransferRate] = std::uint32_t(
        1000000000 * multiplier);
    jv[jss::TransactionType] = jss::AccountSet;
    return jv;
}

} 
} 
} 
























