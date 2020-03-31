

#ifndef RIPPLE_PROTOCOL_SYSTEMPARAMETERS_H_INCLUDED
#define RIPPLE_PROTOCOL_SYSTEMPARAMETERS_H_INCLUDED

#include <cstdint>
#include <string>

namespace ripple {



static inline
std::string const&
systemName ()
{
    static std::string const name = "sgy";
    return name;
}


static
std::uint64_t const
SYSTEM_CURRENCY_GIFT = 1;

static
std::uint64_t const
SYSTEM_CURRENCY_USERS = 2100000000;


static
std::uint64_t const
SYSTEM_CURRENCY_PARTS = 1000000;


static
std::uint64_t const
SYSTEM_CURRENCY_START = SYSTEM_CURRENCY_GIFT * SYSTEM_CURRENCY_USERS * SYSTEM_CURRENCY_PARTS;


static inline
std::string const&
systemCurrencyCode ()
{
    static std::string const code = "SGY";
    return code;
}


static
std::uint32_t constexpr
XRP_LEDGER_EARLIEST_SEQ {32570};

} 

#endif








