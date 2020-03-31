
#ifndef BEAST_UNIT_TEST_GLOBAL_SUITES_HPP
#define BEAST_UNIT_TEST_GLOBAL_SUITES_HPP

#include <beast/unit_test/suite_list.hpp>

namespace beast {
namespace unit_test {

namespace detail {

inline
suite_list&
global_suites()
{
    static suite_list s;
    return s;
}

template<class Suite>
struct insert_suite
{
    insert_suite(char const* name, char const* module,
        char const* library, bool manual, int priority)
    {
        global_suites().insert<Suite>(
            name, module, library, manual, priority);
    }
};

} 

inline
suite_list const&
global_suites()
{
    return detail::global_suites();
}

} 
} 

#endif






