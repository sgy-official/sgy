

#ifndef RIPPLE_PROTOCOL_BOOK_H_INCLUDED
#define RIPPLE_PROTOCOL_BOOK_H_INCLUDED

#include <ripple/protocol/Issue.h>
#include <boost/utility/base_from_member.hpp>

namespace ripple {


class Book
{
public:
    Issue in;
    Issue out;

    Book ()
    {
    }

    Book (Issue const& in_, Issue const& out_)
        : in (in_)
        , out (out_)
    {
    }
};

bool
isConsistent (Book const& book);

std::string
to_string (Book const& book);

std::ostream&
operator<< (std::ostream& os, Book const& x);

template <class Hasher>
void
hash_append (Hasher& h, Book const& b)
{
    using beast::hash_append;
    hash_append(h, b.in, b.out);
}

Book
reversed (Book const& book);


int
compare (Book const& lhs, Book const& rhs);



bool
operator== (Book const& lhs, Book const& rhs);
bool
operator!= (Book const& lhs, Book const& rhs);




bool
operator< (Book const& lhs,Book const& rhs);
bool
operator> (Book const& lhs, Book const& rhs);
bool
operator>= (Book const& lhs, Book const& rhs);
bool
operator<= (Book const& lhs, Book const& rhs);


}


namespace std {

template <>
struct hash <ripple::Issue>
    : private boost::base_from_member <std::hash <ripple::Currency>, 0>
    , private boost::base_from_member <std::hash <ripple::AccountID>, 1>
{
private:
    using currency_hash_type = boost::base_from_member <
        std::hash <ripple::Currency>, 0>;
    using issuer_hash_type = boost::base_from_member <
        std::hash <ripple::AccountID>, 1>;

public:
    explicit hash() = default;

    using value_type = std::size_t;
    using argument_type = ripple::Issue;

    value_type operator() (argument_type const& value) const
    {
        value_type result (currency_hash_type::member (value.currency));
        if (!isXRP (value.currency))
            boost::hash_combine (result,
                issuer_hash_type::member (value.account));
        return result;
    }
};


template <>
struct hash <ripple::Book>
{
private:
    using hasher = std::hash <ripple::Issue>;

    hasher m_hasher;

public:
    explicit hash() = default;

    using value_type = std::size_t;
    using argument_type = ripple::Book;

    value_type operator() (argument_type const& value) const
    {
        value_type result (m_hasher (value.in));
        boost::hash_combine (result, m_hasher (value.out));
        return result;
    }
};

}


namespace boost {

template <>
struct hash <ripple::Issue>
    : std::hash <ripple::Issue>
{
    explicit hash() = default;

    using Base = std::hash <ripple::Issue>;
};

template <>
struct hash <ripple::Book>
    : std::hash <ripple::Book>
{
    explicit hash() = default;

    using Base = std::hash <ripple::Book>;
};

}

#endif








