

#ifndef RIPPLE_TEST_JTX_AMOUNT_H_INCLUDED
#define RIPPLE_TEST_JTX_AMOUNT_H_INCLUDED

#include <test/jtx/Account.h>
#include <test/jtx/amount.h>
#include <test/jtx/tags.h>
#include <ripple/protocol/Issue.h>
#include <ripple/protocol/STAmount.h>
#include <ripple/basics/contract.h>
#include <cstdint>
#include <ostream>
#include <string>
#include <type_traits>

namespace ripple {
namespace test {
namespace jtx {



struct AnyAmount;

struct None
{
    Issue issue;
};


template <class T>
struct dropsPerXRP
{
    static T const value = 1000000;
};


struct PrettyAmount
{
private:
    STAmount amount_;
    std::string name_;

public:
    PrettyAmount() = default;
    PrettyAmount (PrettyAmount const&) = default;
    PrettyAmount& operator=(PrettyAmount const&) = default;

    PrettyAmount (STAmount const& amount,
            std::string const& name)
        : amount_(amount)
        , name_(name)
    {
    }

    
    template <class T>
    PrettyAmount (T v, std::enable_if_t<
        sizeof(T) >= sizeof(int) &&
            std::is_integral<T>::value &&
                std::is_signed<T>::value>* = nullptr)
        : amount_((v > 0) ?
            v : -v, v < 0)
    {
    }

    
    template <class T>
    PrettyAmount (T v, std::enable_if_t<
        sizeof(T) >= sizeof(int) &&
            std::is_integral<T>::value &&
                std::is_unsigned<T>::value>* = nullptr)
        : amount_(v)
    {
    }

    std::string const&
    name() const
    {
        return name_;
    }

    STAmount const&
    value() const
    {
        return amount_;
    }

    operator STAmount const&() const
    {
        return amount_;
    }

    operator AnyAmount() const;
};

inline
bool
operator== (PrettyAmount const& lhs,
    PrettyAmount const& rhs)
{
    return lhs.value() == rhs.value();
}

std::ostream&
operator<< (std::ostream& os,
    PrettyAmount const& amount);


struct BookSpec
{
    AccountID account;
    ripple::Currency currency;

    BookSpec(AccountID const& account_,
        ripple::Currency const& currency_)
        : account(account_)
        , currency(currency_)
    {
    }
};


struct XRP_t
{
    
    operator Issue() const
    {
        return xrpIssue();
    }

    
    
    template <class T, class = std::enable_if_t<
        std::is_integral<T>::value>>
    PrettyAmount
    operator()(T v) const
    {
        return { std::conditional_t<
            std::is_signed<T>::value,
                std::int64_t, std::uint64_t>{v} *
                    dropsPerXRP<T>::value };
    }

    PrettyAmount
    operator()(double v) const
    {
        auto const c =
            dropsPerXRP<int>::value;
        if (v >= 0)
        {
            auto const d = std::uint64_t(
                std::round(v * c));
            if (double(d) / c != v)
                Throw<std::domain_error> (
                    "unrepresentable");
            return { d };
        }
        auto const d = std::int64_t(
            std::round(v * c));
        if (double(d) / c != v)
            Throw<std::domain_error> (
                "unrepresentable");
        return { d };
    }
    

    
    None
    operator()(none_t) const
    {
        return { xrpIssue() };
    }

    friend
    BookSpec
    operator~ (XRP_t const&)
    {        
        return BookSpec( xrpAccount(),
            xrpCurrency() );
    }
};


extern XRP_t const XRP;


template <class Integer,
    class = std::enable_if_t<
        std::is_integral<Integer>::value>>
PrettyAmount
drops (Integer i)
{
    return { i };
}


namespace detail {

struct epsilon_multiple
{
    std::size_t n;
};

} 

struct epsilon_t
{
    epsilon_t()
    {
    }

    detail::epsilon_multiple
    operator()(std::size_t n) const
    {
        return { n };
    }
};

static epsilon_t const epsilon;


class IOU
{
public:
    Account account;
    ripple::Currency currency;

    IOU(Account const& account_,
            ripple::Currency const& currency_)
        : account(account_)
        , currency(currency_)
    {
    }

    Issue
    issue() const
    {
        return { currency, account.id() };
    }

    
    operator Issue() const
    {
        return issue();
    }

    template <class T, class = std::enable_if_t<
        sizeof(T) >= sizeof(int) &&
            std::is_arithmetic<T>::value>>
    PrettyAmount operator()(T v) const
    {
        return { amountFromString(issue(),
            std::to_string(v)), account.name() };
    }

    PrettyAmount operator()(epsilon_t) const;
    PrettyAmount operator()(detail::epsilon_multiple) const;


    
    None operator()(none_t) const
    {
        return { issue() };
    }

    friend
    BookSpec
    operator~ (IOU const& iou)
    {
        return BookSpec(iou.account.id(), iou.currency);
    }
};

std::ostream&
operator<<(std::ostream& os,
    IOU const& iou);


struct any_t
{
    inline
    AnyAmount
    operator()(STAmount const& sta) const;
};


struct AnyAmount
{
    bool is_any;
    STAmount value;

    AnyAmount() = delete;
    AnyAmount (AnyAmount const&) = default;
    AnyAmount& operator= (AnyAmount const&) = default;

    AnyAmount (STAmount const& amount)
        : is_any(false)
        , value(amount)
    {
    }

    AnyAmount (STAmount const& amount,
            any_t const*)
        : is_any(true)
        , value(amount)
    {
    }

    void
    to (AccountID const& id)
    {
        if (! is_any)
            return;
        value.setIssuer(id);
    }
};

inline
AnyAmount
any_t::operator()(STAmount const& sta) const
{
    return AnyAmount(sta, this);
}


extern any_t const any;

} 
} 
} 

#endif








