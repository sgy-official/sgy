

#ifndef RIPPLE_PROTOCOL_IOUAMOUNT_H_INCLUDED
#define RIPPLE_PROTOCOL_IOUAMOUNT_H_INCLUDED

#include <ripple/beast/utility/Zero.h>
#include <boost/operators.hpp>
#include <cstdint>
#include <string>
#include <utility>

namespace ripple {


class IOUAmount
    : private boost::totally_ordered<IOUAmount>
    , private boost::additive <IOUAmount>
{
private:
    std::int64_t mantissa_;
    int exponent_;

    
    void
    normalize ();

public:
    IOUAmount () = default;
    IOUAmount (IOUAmount const& other) = default;
    IOUAmount&operator= (IOUAmount const& other) = default;

    IOUAmount (beast::Zero)
    {
        *this = beast::zero;
    }

    IOUAmount (std::int64_t mantissa, int exponent)
        : mantissa_ (mantissa)
        , exponent_ (exponent)
    {
        normalize ();
    }

    IOUAmount&
    operator= (beast::Zero)
    {
        mantissa_ = 0;
        exponent_ = -100;
        return *this;
    }

    IOUAmount&
    operator+= (IOUAmount const& other);

    IOUAmount&
    operator-= (IOUAmount const& other)
    {
        *this += -other;
        return *this;
    }

    IOUAmount
    operator- () const
    {
        return { -mantissa_, exponent_ };
    }

    bool
    operator==(IOUAmount const& other) const
    {
        return exponent_ == other.exponent_ &&
            mantissa_ == other.mantissa_;
    }

    bool
    operator<(IOUAmount const& other) const;

    
    explicit
    operator bool() const noexcept
    {
        return mantissa_ != 0;
    }

    
    int
    signum() const noexcept
    {
        return (mantissa_ < 0) ? -1 : (mantissa_ ? 1 : 0);
    }

    int
    exponent() const noexcept
    {
        return exponent_;
    }

    std::int64_t
    mantissa() const noexcept
    {
        return mantissa_;
    }
};

std::string
to_string (IOUAmount const& amount);


IOUAmount
mulRatio (
    IOUAmount const& amt,
    std::uint32_t num,
    std::uint32_t den,
    bool roundUp);

}

#endif








