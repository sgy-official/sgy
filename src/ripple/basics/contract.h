

#ifndef RIPPLE_BASICS_CONTRACT_H_INCLUDED
#define RIPPLE_BASICS_CONTRACT_H_INCLUDED

#include <ripple/beast/type_name.h>
#include <exception>
#include <string>
#include <typeinfo>
#include <utility>

namespace ripple {




void
LogThrow (std::string const& title);


[[noreturn]]
inline
void
Rethrow ()
{
    LogThrow ("Re-throwing exception");
    throw;
}

template <class E, class... Args>
[[noreturn]]
inline
void
Throw (Args&&... args)
{
    static_assert (std::is_convertible<E*, std::exception*>::value,
        "Exception must derive from std::exception.");

    E e(std::forward<Args>(args)...);
    LogThrow (std::string("Throwing exception of type " +
                          beast::type_name<E>() +": ") + e.what());
    throw e;
}


[[noreturn]]
void
LogicError (
    std::string const& how) noexcept;

} 

#endif








