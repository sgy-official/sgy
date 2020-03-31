

#include <ripple/protocol/Issue.h>

namespace ripple {

bool
isConsistent (Issue const& ac)
{
    return isXRP (ac.currency) == isXRP (ac.account);
}

std::string
to_string (Issue const& ac)
{
    if (isXRP (ac.account))
        return to_string (ac.currency);

    return to_string(ac.account) + "/" + to_string(ac.currency);
}

std::ostream&
operator<< (std::ostream& os, Issue const& x)
{
    os << to_string (x);
    return os;
}


int
compare (Issue const& lhs, Issue const& rhs)
{
    int diff = compare (lhs.currency, rhs.currency);
    if (diff != 0)
        return diff;
    if (isXRP (lhs.currency))
        return 0;
    return compare (lhs.account, rhs.account);
}



bool
operator== (Issue const& lhs, Issue const& rhs)
{
    return compare (lhs, rhs) == 0;
}

bool
operator!= (Issue const& lhs, Issue const& rhs)
{
    return ! (lhs == rhs);
}




bool
operator< (Issue const& lhs, Issue const& rhs)
{
    return compare (lhs, rhs) < 0;
}

bool
operator> (Issue const& lhs, Issue const& rhs)
{
    return rhs < lhs;
}

bool
operator>= (Issue const& lhs, Issue const& rhs)
{
    return ! (lhs < rhs);
}

bool
operator<= (Issue const& lhs, Issue const& rhs)
{
    return ! (rhs < lhs);
}

} 
























