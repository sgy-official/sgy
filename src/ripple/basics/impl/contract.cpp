

#include <ripple/basics/contract.h>
#include <ripple/basics/Log.h>
#include <cstdlib>
#include <exception>
#include <iostream>

namespace ripple {

namespace detail {

[[noreturn]]
void
accessViolation() noexcept
{
    int volatile* j = 0;
    (void)*j;
    std::abort ();
}

} 

void
LogThrow (std::string const& title)
{
    JLOG(debugLog().warn()) << title;
}

[[noreturn]]
void
LogicError (std::string const& s) noexcept
{
    JLOG(debugLog().fatal()) << s;
    std::cerr << "Logic error: " << s << std::endl;
    detail::accessViolation();
}

} 
























