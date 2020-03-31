

#ifndef RIPPLE_PROTOCOL_QUALITY_H_INCLUDED
#define RIPPLE_PROTOCOL_QUALITY_H_INCLUDED

#include <ripple/protocol/AmountConversions.h>
#include <ripple/protocol/IOUAmount.h>
#include <ripple/protocol/STAmount.h>
#include <ripple/protocol/XRPAmount.h>

#include <cstdint>
#include <ostream>

namespace ripple {


template<class In, class Out>
struct TAmounts
{
    TAmounts() = default;

    TAmounts (beast::Zero, beast::Zero)
        : in (beast::zero)
        , out (beast::zero)
    {
    }

    TAmounts (In const& in_, Out const& out_)
        : in (in_)
        , out (out_)
    {
    }

    
    bool
    empty() const noexcept
    {
        return in <= beast::zero || out <= beast::zero;
    }

    TAmounts& operator+=(TAmounts const& rhs)
    {
        in += rhs.in;
        out += rhs.out;
        return *this;
    }

    TAmounts& operator-=(TAmounts const& rhs)
    {
        in -= rhs.in;
        out -= rhs.out;
        return *this;
    }

    In in;
    Out out;
};

template<class In, class Out>
TAmounts<In, Out> make_Amounts(In const& in, Out const& out)
{
    return TAmounts<In, Out>(in, out);
}

using Amounts = TAmounts<STAmount, STAmount>;

template<class In, class Out>
bool
operator== (
    TAmounts<In, Out> const& lhs,
    TAmounts<In, Out> const& rhs) noexcept
{
    return lhs.in == rhs.in && lhs.out == rhs.out;
}

template<class In, class Out>
bool
operator!= (
    TAmounts<In, Out> const& lhs,
    TAmounts<In, Out> const& rhs) noexcept
{
    return ! (lhs == rhs);
}


#define QUALITY_ONE 1000000000


class Quality
{
public:
    using value_type = std::uint64_t;

    static const int minTickSize = 3;
    static const int maxTickSize = 16;

private:
    value_type m_value;

public:
    Quality() = default;

    
    explicit
    Quality (std::uint64_t value);

    
    explicit
    Quality (Amounts const& amount);

    
    template<class In, class Out>
    Quality (Out const& out, In const& in)
        : Quality (Amounts (toSTAmount (in),
                            toSTAmount (out)))
    {}

    
    
    Quality&
    operator++();

    Quality
    operator++ (int);
    

    
    
    Quality&
    operator--();

    Quality
    operator-- (int);
    

    
    STAmount
    rate () const
    {
        return amountFromQuality (m_value);
    }

    
    Quality
    round (int tickSize) const;

    
    Amounts
    ceil_in (Amounts const& amount, STAmount const& limit) const;

    template<class In, class Out>
    TAmounts<In, Out>
    ceil_in (TAmounts<In, Out> const& amount, In const& limit) const
    {
        if (amount.in <= limit)
            return amount;

        Amounts stAmt (toSTAmount (amount.in), toSTAmount (amount.out));
        STAmount stLim (toSTAmount (limit));
        auto const stRes = ceil_in (stAmt, stLim);
        return TAmounts<In, Out> (toAmount<In> (stRes.in), toAmount<Out> (stRes.out));
    }

    
    Amounts
    ceil_out (Amounts const& amount, STAmount const& limit) const;

    template<class In, class Out>
    TAmounts<In, Out>
    ceil_out (TAmounts<In, Out> const& amount, Out const& limit) const
    {
        if (amount.out <= limit)
            return amount;

        Amounts stAmt (toSTAmount (amount.in), toSTAmount (amount.out));
        STAmount stLim (toSTAmount (limit));
        auto const stRes = ceil_out (stAmt, stLim);
        return TAmounts<In, Out> (toAmount<In> (stRes.in), toAmount<Out> (stRes.out));
    }

    
    friend
    bool
    operator< (Quality const& lhs, Quality const& rhs) noexcept
    {
        return lhs.m_value > rhs.m_value;
    }

    friend
    bool
    operator> (Quality const& lhs, Quality const& rhs) noexcept
    {
        return lhs.m_value < rhs.m_value;
    }

    friend
    bool
    operator<= (Quality const& lhs, Quality const& rhs) noexcept
    {
        return !(lhs > rhs);
    }

    friend
    bool
    operator>= (Quality const& lhs, Quality const& rhs) noexcept
    {
        return !(lhs < rhs);
    }

    friend
    bool
    operator== (Quality const& lhs, Quality const& rhs) noexcept
    {
        return lhs.m_value == rhs.m_value;
    }

    friend
    bool
    operator!= (Quality const& lhs, Quality const& rhs) noexcept
    {
        return ! (lhs == rhs);
    }

    friend
    std::ostream&
    operator<< (std::ostream& os, Quality const& quality)
    {
        os << quality.m_value;
        return os;
    }
};


Quality
composed_quality (Quality const& lhs, Quality const& rhs);

}

#endif








