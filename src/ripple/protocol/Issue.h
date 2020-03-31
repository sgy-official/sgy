

#ifndef RIPPLE_PROTOCOL_ISSUE_H_INCLUDED
#define RIPPLE_PROTOCOL_ISSUE_H_INCLUDED

#include <cassert>
#include <functional>
#include <type_traits>

#include <ripple/protocol/UintTypes.h>

namespace ripple {


class Issue
{
public:
    Currency currency;
    AccountID account;

    Issue ()
    {
    }

    Issue (Currency const& c, AccountID const& a)
            : currency (c), account (a)
    {
    }
};

bool
isConsistent (Issue const& ac);

std::string
to_string (Issue const& ac);

std::ostream&
operator<< (std::ostream& os, Issue const& x);

template <class Hasher>
void
hash_append(Hasher& h, Issue const& r)
{
    using beast::hash_append;
    hash_append(h, r.currency, r.account);
}


int
compare (Issue const& lhs, Issue const& rhs);



bool
operator== (Issue const& lhs, Issue const& rhs);
bool
operator!= (Issue const& lhs, Issue const& rhs);




bool
operator< (Issue const& lhs, Issue const& rhs);
bool
operator> (Issue const& lhs, Issue const& rhs);
bool
operator>= (Issue const& lhs, Issue const& rhs);
bool
operator<= (Issue const& lhs, Issue const& rhs);




inline Issue const& xrpIssue ()
{
    static Issue issue {xrpCurrency(), xrpAccount()};
    return issue;
}


inline Issue const& noIssue ()
{
    static Issue issue {noCurrency(), noAccount()};
    return issue;
}

}

#endif








