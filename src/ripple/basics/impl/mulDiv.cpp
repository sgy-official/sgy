

#include <ripple/basics/mulDiv.h>
#include <ripple/basics/contract.h>
#include <boost/multiprecision/cpp_int.hpp>
#include <limits>
#include <utility>

namespace ripple
{

std::pair<bool, std::uint64_t>
mulDiv(std::uint64_t value, std::uint64_t mul, std::uint64_t div)
{
    using namespace boost::multiprecision;

    uint128_t result;
    result = multiply(result, value, mul);

    result /= div;

    auto const limit = std::numeric_limits<std::uint64_t>::max();

    if (result > limit)
        return { false, limit };

    return { true, static_cast<std::uint64_t>(result) };
}

} 
























