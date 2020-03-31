

#ifndef RIPPLE_BASICS_MULDIV_H_INCLUDED
#define RIPPLE_BASICS_MULDIV_H_INCLUDED

#include <cstdint>
#include <utility>

namespace ripple
{


std::pair<bool, std::uint64_t>
mulDiv(std::uint64_t value, std::uint64_t mul, std::uint64_t div);

} 

#endif








