

#ifndef RIPPLE_TEST_JTX_OFFER_H_INCLUDED
#define RIPPLE_TEST_JTX_OFFER_H_INCLUDED

#include <test/jtx/Account.h>
#include <ripple/json/json_value.h>
#include <ripple/protocol/STAmount.h>

namespace ripple {
namespace test {
namespace jtx {


Json::Value
offer (Account const& account,
    STAmount const& in, STAmount const& out, std::uint32_t flags = 0);


Json::Value
offer_cancel (Account const& account, std::uint32_t offerSeq);

} 
} 
} 

#endif








