

#include <test/jtx/offer.h>
#include <ripple/protocol/jss.h>

namespace ripple {
namespace test {
namespace jtx {

Json::Value
offer (Account const& account,
    STAmount const& in, STAmount const& out,  std::uint32_t flags)
{
    Json::Value jv;
    jv[jss::Account] = account.human();
    jv[jss::TakerPays] = in.getJson(JsonOptions::none);
    jv[jss::TakerGets] = out.getJson(JsonOptions::none);
    if (flags)
        jv[jss::Flags] = flags;
    jv[jss::TransactionType] = jss::OfferCreate;
    return jv;
}

Json::Value
offer_cancel (Account const& account, std::uint32_t offerSeq)
{
    Json::Value jv;
    jv[jss::Account] = account.human();
    jv[jss::OfferSequence] = offerSeq;
    jv[jss::TransactionType] = jss::OfferCancel;
    return jv;
}

} 
} 
} 
























