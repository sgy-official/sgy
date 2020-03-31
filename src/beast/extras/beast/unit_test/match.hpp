
#ifndef BEAST_UNIT_TEST_MATCH_HPP
#define BEAST_UNIT_TEST_MATCH_HPP

#include <beast/unit_test/suite_info.hpp>
#include <string>

namespace beast {
namespace unit_test {

class selector
{
public:
    enum mode_t
    {
        all,

        automatch,

        suite,

        library,

        module,

        none
    };

private:
    mode_t mode_;
    std::string pat_;
    std::string library_;

public:
    template<class = void>
    explicit
    selector(mode_t mode, std::string const& pattern = "");

    template<class = void>
    bool
    operator()(suite_info const& s);
};


template<class>
selector::selector(mode_t mode, std::string const& pattern)
    : mode_(mode)
    , pat_(pattern)
{
    if(mode_ == automatch && pattern.empty())
        mode_ = all;
}

template<class>
bool
selector::operator()(suite_info const& s)
{
    switch(mode_)
    {
    case automatch:
        if(s.name() == pat_ || s.full_name() == pat_)
        {
            mode_ = none;
            return true;
        }

        if(pat_ == s.module())
        {
            mode_ = module;
            library_ = s.library();
            return ! s.manual();
        }

        if(pat_ == s.library())
        {
            mode_ = library;
            return ! s.manual();
        }

        return false;

    case suite:
        return pat_ == s.name();

    case module:
        return pat_ == s.module() && ! s.manual();

    case library:
        return pat_ == s.library() && ! s.manual();

    case none:
        return false;

    case all:
    default:
        break;
    };

    return ! s.manual();
}




inline
selector
match_auto(std::string const& name)
{
    return selector(selector::automatch, name);
}


inline
selector
match_all()
{
    return selector(selector::all);
}


inline
selector
match_suite(std::string const& name)
{
    return selector(selector::suite, name);
}


inline
selector
match_library(std::string const& name)
{
    return selector(selector::library, name);
}

} 
} 

#endif






