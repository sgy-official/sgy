
#ifndef BEAST_UNIT_TEST_SUITE_LIST_HPP
#define BEAST_UNIT_TEST_SUITE_LIST_HPP

#include <beast/unit_test/suite_info.hpp>
#include <beast/unit_test/detail/const_container.hpp>
#include <boost/assert.hpp>
#include <typeindex>
#include <set>
#include <unordered_set>

namespace beast {
namespace unit_test {

class suite_list
    : public detail::const_container <std::set <suite_info>>
{
private:
#ifndef NDEBUG
    std::unordered_set<std::string> names_;
    std::unordered_set<std::type_index> classes_;
#endif

public:
    
    template<class Suite>
    void
    insert(
        char const* name,
        char const* module,
        char const* library,
        bool manual,
        int priority);
};


template<class Suite>
void
suite_list::insert(
    char const* name,
    char const* module,
    char const* library,
    bool manual,
    int priority)
{
#ifndef NDEBUG
    {
        std::string s;
        s = std::string(library) + "." + module + "." + name;
        auto const result(names_.insert(s));
        BOOST_ASSERT(result.second); 
    }

    {
        auto const result(classes_.insert(
            std::type_index(typeid(Suite))));
        BOOST_ASSERT(result.second); 
    }
#endif
    cont().emplace(make_suite_info<Suite>(
        name, module, library, manual, priority));
}

} 
} 

#endif







