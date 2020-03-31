

#ifndef RIPPLE_PROTOCOL_XRPAMOUNT_H_INCLUDED
#define RIPPLE_PROTOCOL_XRPAMOUNT_H_INCLUDED

#include <ripple/basics/contract.h>
#include <ripple/protocol/SystemParameters.h>
#include <ripple/beast/utility/Zero.h>
#include <boost/operators.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <cstdint>
#include <string>
#include <type_traits>

namespace ripple {

class XRPAmount
    : private boost::totally_ordered <XRPAmount>
    , private boost::additive <XRPAmount>
{
private:
    std::int64_t drops_;

public:
    XRPAmount () = default;
    XRPAmount (XRPAmount const& other) = default;
    XRPAmount& operator= (XRPAmount const& other) = default;

    XRPAmount (beast::Zero)
        : drops_ (0)
    {
    }

    XRPAmount&
    operator= (beast::Zero)
    {
        drops_ = 0;
        return *this;
    }

    template <class Integer,
        class = typename std::enable_if_t <
            std::is_integral<Integer>::value>>
    XRPAmount (Integer drops)
        : drops_ (static_cast<std::int64_t> (drops))
    {
    }

    template <class Integer,
        class = typename std::enable_if_t <
            std::is_integral<Integer>::value>>
    XRPAmount&
    operator= (Integer drops)
    {
        drops_ = static_cast<std::int64_t> (drops);
        return *this;
    }

    XRPAmount&
    operator+= (XRPAmount const& other)
    {
        drops_ += other.drops_;
        return *this;
    }

    XRPAmount&
    operator-= (XRPAmount const& other)
    {
        drops_ -= other.drops_;
        return *this;
    }

    XRPAmount
    operator- () const
    {
        return { -drops_ };
    }

    bool
    operator==(XRPAmount const& other) const
    {
        return drops_ == other.drops_;
    }

    bool
    operator<(XRPAmount const& other) const
    {
        return drops_ < other.drops_;
    }

    
    explicit
    operator bool() const noexcept
    {
        return drops_ != 0;
    }

    
    int
    signum() const noexcept
    {
        return (drops_ < 0) ? -1 : (drops_ ? 1 : 0);
    }

    
    std::int64_t
    drops () const
    {
        return drops_;
    }
};

inline
std::string
to_string (XRPAmount const& amount)
{
    return std::to_string (amount.drops ());
}

inline
XRPAmount
mulRatio (
    XRPAmount const& amt,
    std::uint32_t num,
    std::uint32_t den,
    bool roundUp)
{
    using namespace boost::multiprecision;

    if (!den)
        Throw<std::runtime_error> ("division by zero");

    int128_t const amt128 (amt.drops ());
    auto const neg = amt.drops () < 0;
    auto const m = amt128 * num;
    auto r = m / den;
    if (m % den)
    {
        if (!neg && roundUp)
            r += 1;
        if (neg && !roundUp)
            r -= 1;
    }
    if (r > std::numeric_limits<std::int64_t>::max ())
        Throw<std::overflow_error> ("XRP mulRatio overflow");
    return XRPAmount (r.convert_to<std::int64_t> ());
}


inline
bool isLegalAmount (XRPAmount const& amount)
{
    return amount.drops () <= SYSTEM_CURRENCY_START;
}

}

#endif








