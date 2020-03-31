

#ifndef RIPPLE_BASICS_BASICCONFIG_H_INCLUDED
#define RIPPLE_BASICS_BASICCONFIG_H_INCLUDED

#include <ripple/basics/contract.h>
#include <beast/unit_test/detail/const_container.hpp>
#include <boost/beast/core/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>
#include <map>
#include <ostream>
#include <string>
#include <vector>

namespace ripple {

using IniFileSections = std::map<std::string, std::vector<std::string>>;



class Section
    : public beast::unit_test::detail::const_container <
        std::map <std::string, std::string, boost::beast::iless>>
{
private:
    std::string name_;
    std::vector <std::string> lines_;
    std::vector <std::string> values_;

public:
    
    explicit
    Section (std::string const& name = "");

    
    std::string const&
    name() const
    {
        return name_;
    }

    
    std::vector <std::string> const&
    lines() const
    {
        return lines_;
    }

    
    std::vector <std::string> const&
    values() const
    {
        return values_;
    }

    
    void
    legacy (std::string value)
    {
        if (lines_.empty ())
            lines_.emplace_back (std::move (value));
        else
            lines_[0] = std::move (value);
    }

    
    std::string
    legacy () const
    {
        if (lines_.empty ())
            return "";
        if (lines_.size () > 1)
            Throw<std::runtime_error> (
                "A legacy value must have exactly one line. Section: " + name_);
        return lines_[0];
    }

    
    void
    set (std::string const& key, std::string const& value);

    
    void
    append (std::vector <std::string> const& lines);

    
    void
    append (std::string const& line)
    {
        append (std::vector<std::string>{ line });
    }

    
    bool
    exists (std::string const& name) const;

    
    std::pair <std::string, bool>
    find (std::string const& name) const;

    template <class T>
    boost::optional<T>
    get (std::string const& name) const
    {
        auto const iter = cont().find(name);
        if (iter == cont().end())
            return boost::none;
        return boost::lexical_cast<T>(iter->second);
    }

    template<class T>
    T
    value_or(std::string const& name, T const& other) const
    {
        auto const iter = cont().find(name);
        if (iter == cont().end())
            return other;
        return boost::lexical_cast<T>(iter->second);
    }

    friend
    std::ostream&
    operator<< (std::ostream&, Section const& section);
};



class BasicConfig
{
private:
    std::map <std::string, Section, boost::beast::iless> map_;

public:
    
    bool
    exists (std::string const& name) const;

    
    
    Section&
    section (std::string const& name);

    Section const&
    section (std::string const& name) const;

    Section const&
    operator[] (std::string const& name) const
    {
        return section(name);
    }

    Section&
    operator[] (std::string const& name)
    {
        return section(name);
    }
    

    
    void
    overwrite (std::string const& section, std::string const& key,
        std::string const& value);

    
    void
    deprecatedClearSection (std::string const& section);

    
    void
    legacy(std::string const& section, std::string value);

    
    std::string
    legacy(std::string const& sectionName) const;

    friend
    std::ostream&
    operator<< (std::ostream& ss, BasicConfig const& c);

protected:
    void
    build (IniFileSections const& ifs);
};



template <class T>
bool
set (T& target, std::string const& name, Section const& section)
{
    auto const result = section.find (name);
    if (! result.second)
        return false;
    try
    {
        target = boost::lexical_cast <T> (result.first);
        return true;
    }
    catch (boost::bad_lexical_cast&)
    {
    }
    return false;
}


template <class T>
bool
set (T& target, T const& defaultValue,
    std::string const& name, Section const& section)
{
    auto const result = section.find (name);
    if (! result.second)
        return false;
    try
    {
        target = boost::lexical_cast <T> (result.first);
        return true;
    }
    catch (boost::bad_lexical_cast&)
    {
        target = defaultValue;
    }
    return false;
}


template <class T>
T
get (Section const& section,
    std::string const& name, T const& defaultValue = T{})
{
    auto const result = section.find (name);
    if (! result.second)
        return defaultValue;
    try
    {
        return boost::lexical_cast <T> (result.first);
    }
    catch (boost::bad_lexical_cast&)
    {
    }
    return defaultValue;
}

inline
std::string
get (Section const& section,
    std::string const& name, const char* defaultValue)
{
    auto const result = section.find (name);
    if (! result.second)
        return defaultValue;
    try
    {
        return boost::lexical_cast <std::string> (result.first);
    }
    catch(std::exception const&)
    {
    }
    return defaultValue;
}

template <class T>
bool
get_if_exists (Section const& section,
    std::string const& name, T& v)
{
    auto const result = section.find (name);
    if (! result.second)
        return false;
    try
    {
        v = boost::lexical_cast <T> (result.first);
        return true;
    }
    catch (boost::bad_lexical_cast&)
    {
    }
    return false;
}

template <>
inline
bool
get_if_exists<bool> (Section const& section,
    std::string const& name, bool& v)
{
    int intVal = 0;
    if (get_if_exists (section, name, intVal))
    {
        v = bool (intVal);
        return true;
    }
    return false;
}

} 

#endif








