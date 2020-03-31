

#include <test/jtx/trust.h>
#include <ripple/protocol/jss.h>
#include <ripple/basics/contract.h>
#include <stdexcept>

namespace ripple {
namespace test {
namespace jtx {

Json::Value
trust (Account const& account,
    STAmount const& amount,
        std::uint32_t flags)
{
    if (isXRP(amount))
        Throw<std::runtime_error> (
            "trust() requires IOU");
    Json::Value jv;
    jv[jss::Account] = account.human();
    jv[jss::LimitAmount] = amount.getJson(JsonOptions::none);
    jv[jss::TransactionType] = jss::TrustSet;
    jv[jss::Flags] = flags;
    return jv;
}

Json::Value
trust (Account const& account,
    STAmount const& amount,
    Account const& peer,
    std::uint32_t flags)
{
    if (isXRP(amount))
        Throw<std::runtime_error> (
            "trust() requires IOU");
    Json::Value jv;
    jv[jss::Account] = account.human();
    {
        auto& ja = jv[jss::LimitAmount] = amount.getJson(JsonOptions::none);
        ja[jss::issuer] = peer.human();
    }
    jv[jss::TransactionType] = jss::TrustSet;
    jv[jss::Flags] = flags;
    return jv;
}


} 
} 
} 
























