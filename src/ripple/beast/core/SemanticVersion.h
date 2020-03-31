

#ifndef BEAST_MODULE_CORE_DIAGNOSTIC_SEMANTICVERSION_H_INCLUDED
#define BEAST_MODULE_CORE_DIAGNOSTIC_SEMANTICVERSION_H_INCLUDED

#include <vector>
#include <string>

namespace beast {


class SemanticVersion
{
public:
    using identifier_list = std::vector<std::string>;

    int majorVersion;
    int minorVersion;
    int patchVersion;

    identifier_list preReleaseIdentifiers;
    identifier_list metaData;

    SemanticVersion ();

    SemanticVersion (std::string const& version);

    
    bool parse (std::string const& input);

    
    std::string print () const;

    inline bool isRelease () const noexcept
    {
        return preReleaseIdentifiers.empty();
    }
    inline bool isPreRelease () const noexcept
    {
        return !isRelease ();
    }
};


int compare (SemanticVersion const& lhs, SemanticVersion const& rhs);

inline bool
operator== (SemanticVersion const& lhs, SemanticVersion const& rhs)
{
    return compare (lhs, rhs) == 0;
}

inline bool
operator!= (SemanticVersion const& lhs, SemanticVersion const& rhs)
{
    return compare (lhs, rhs) != 0;
}

inline bool
operator>= (SemanticVersion const& lhs, SemanticVersion const& rhs)
{
    return compare (lhs, rhs) >= 0;
}

inline bool
operator<= (SemanticVersion const& lhs, SemanticVersion const& rhs)
{
    return compare (lhs, rhs) <= 0;
}

inline bool
operator>  (SemanticVersion const& lhs, SemanticVersion const& rhs)
{
    return compare (lhs, rhs) >  0;
}

inline bool
operator<  (SemanticVersion const& lhs, SemanticVersion const& rhs)
{
    return compare (lhs, rhs) <  0;
}

} 

#endif








