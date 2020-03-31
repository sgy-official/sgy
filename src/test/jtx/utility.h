

#ifndef RIPPLE_TEST_JTX_UTILITY_H_INCLUDED
#define RIPPLE_TEST_JTX_UTILITY_H_INCLUDED

#include <test/jtx/Account.h>
#include <ripple/json/json_value.h>
#include <ripple/app/ledger/Ledger.h>
#include <ripple/protocol/STObject.h>
#include <stdexcept>

namespace ripple {
namespace test {
namespace jtx {


struct parse_error : std::logic_error
{
    template <class String>
    explicit
    parse_error (String const& s)
        : logic_error(s)
    {
    }
};


STObject
parse (Json::Value const& jv);


void
sign (Json::Value& jv,
    Account const& account);


void
fill_fee (Json::Value& jv,
    ReadView const& view);


void
fill_seq (Json::Value& jv,
    ReadView const& view);

} 
} 
} 

#endif








