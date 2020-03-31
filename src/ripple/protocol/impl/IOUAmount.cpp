

#include <ripple/basics/contract.h>
#include <ripple/protocol/IOUAmount.h>
#include <boost/multiprecision/cpp_int.hpp>
#include <algorithm>
#include <numeric>
#include <iterator>
#include <stdexcept>

namespace ripple {


static std::int64_t const minMantissa = 1000000000000000ull;
static std::int64_t const maxMantissa = 9999999999999999ull;

static int const minExponent = -96;
static int const maxExponent = 80;

void
IOUAmount::normalize ()
{
    if (mantissa_ == 0)
    {
        *this = beast::zero;
        return;
    }

    bool const negative = (mantissa_ < 0);

    if (negative)
        mantissa_ = -mantissa_;

    while ((mantissa_ < minMantissa) && (exponent_ > minExponent))
    {
        mantissa_ *= 10;
        --exponent_;
    }

    while (mantissa_ > maxMantissa)
    {
        if (exponent_ >= maxExponent)
            Throw<std::overflow_error> ("IOUAmount::normalize");

        mantissa_ /= 10;
        ++exponent_;
    }

    if ((exponent_ < minExponent) || (mantissa_ < minMantissa))
    {
        *this = beast::zero;
        return;
    }

    if (exponent_ > maxExponent)
        Throw<std::overflow_error> ("value overflow");

    if (negative)
        mantissa_ = -mantissa_;
}

IOUAmount&
IOUAmount::operator+= (IOUAmount const& other)
{
    if (other == beast::zero)
        return *this;

    if (*this == beast::zero)
    {
        *this = other;
        return *this;
    }

    auto m = other.mantissa_;
    auto e = other.exponent_;

    while (exponent_ < e)
    {
        mantissa_ /= 10;
        ++exponent_;
    }

    while (e < exponent_)
    {
        m /= 10;
        ++e;
    }

    mantissa_ += m;

    if (mantissa_ >= -10 && mantissa_ <= 10)
    {
        *this = beast::zero;
        return *this;
    }

    normalize ();

    return *this;
}

bool
IOUAmount::operator<(IOUAmount const& other) const
{
    bool const lneg = mantissa_ < 0;
    bool const rneg = other.mantissa_ < 0;

    if (lneg != rneg)
        return lneg;

    if (mantissa_ == 0)
        return other.mantissa_ > 0;

    if (other.mantissa_ == 0)
        return false;

    if (exponent_ > other.exponent_)
        return lneg;
    if (exponent_ < other.exponent_)
        return !lneg;

    return mantissa_ < other.mantissa_;
}

std::string
to_string (IOUAmount const& amount)
{
    if (amount == beast::zero)
        return "0";

    int const exponent = amount.exponent ();
    auto mantissa = amount.mantissa ();

    if (((exponent != 0) && ((exponent < -25) || (exponent > -5))))
    {
        std::string ret = std::to_string (mantissa);
        ret.append (1, 'e');
        ret.append (std::to_string (exponent));
        return ret;
    }

    bool negative = false;

    if (mantissa < 0)
    {
        mantissa = -mantissa;
        negative = true;
    }

    assert (exponent + 43 > 0);

    size_t const pad_prefix = 27;
    size_t const pad_suffix = 23;

    std::string const raw_value (std::to_string (mantissa));
    std::string val;

    val.reserve (raw_value.length () + pad_prefix + pad_suffix);
    val.append (pad_prefix, '0');
    val.append (raw_value);
    val.append (pad_suffix, '0');

    size_t const offset (exponent + 43);

    auto pre_from (val.begin ());
    auto const pre_to (val.begin () + offset);

    auto const post_from (val.begin () + offset);
    auto post_to (val.end ());

    if (std::distance (pre_from, pre_to) > pad_prefix)
        pre_from += pad_prefix;

    assert (post_to >= post_from);

    pre_from = std::find_if (pre_from, pre_to,
        [](char c)
        {
            return c != '0';
        });

    if (std::distance (post_from, post_to) > pad_suffix)
        post_to -= pad_suffix;

    assert (post_to >= post_from);

    post_to = std::find_if(
        std::make_reverse_iterator (post_to),
        std::make_reverse_iterator (post_from),
        [](char c)
        {
            return c != '0';
        }).base();

    std::string ret;

    if (negative)
        ret.append (1, '-');

    if (pre_from == pre_to)
        ret.append (1, '0');
    else
        ret.append(pre_from, pre_to);

    if (post_to != post_from)
    {
        ret.append (1, '.');
        ret.append (post_from, post_to);
    }

    return ret;
}

IOUAmount
mulRatio (
    IOUAmount const& amt,
    std::uint32_t num,
    std::uint32_t den,
    bool roundUp)
{
    using namespace boost::multiprecision;

    if (!den)
        Throw<std::runtime_error> ("division by zero");

    static auto const powerTable = []
    {
        std::vector<uint128_t> result;
        result.reserve (30);  
        uint128_t cur (1);
        for (int i = 0; i < 30; ++i)
        {
            result.push_back (cur);
            cur *= 10;
        };
        return result;
    }();

    static auto log10Floor = [](uint128_t const& v)
    {
        auto const l = std::lower_bound (powerTable.begin (), powerTable.end (), v);
        int index = std::distance (powerTable.begin (), l);
        if (*l != v)
            --index;
        return index;
    };

    static auto log10Ceil = [](uint128_t const& v)
    {
        auto const l = std::lower_bound (powerTable.begin (), powerTable.end (), v);
        return int(std::distance (powerTable.begin (), l));
    };

    static auto const fl64 =
        log10Floor (std::numeric_limits<std::int64_t>::max ());

    bool const neg = amt.mantissa () < 0;
    uint128_t const den128 (den);
    uint128_t const mul =
        uint128_t (neg ? -amt.mantissa () : amt.mantissa ()) * uint128_t (num);

    auto low = mul / den128;
    uint128_t rem (mul - low * den128);

    int exponent = amt.exponent ();

    if (rem)
    {
        auto const roomToGrow = fl64 - log10Ceil (low);
        if (roomToGrow > 0)
        {
            exponent -= roomToGrow;
            low *= powerTable[roomToGrow];
            rem *= powerTable[roomToGrow];
        }
        auto const addRem = rem / den128;
        low += addRem;
        rem = rem - addRem * den128;
    }

    bool hasRem = bool(rem);
    auto const mustShrink = log10Ceil (low) - fl64;
    if (mustShrink > 0)
    {
        uint128_t const sav (low);
        exponent += mustShrink;
        low /= powerTable[mustShrink];
        if (!hasRem)
            hasRem = bool(sav - low * powerTable[mustShrink]);
    }

    std::int64_t mantissa = low.convert_to<std::int64_t> ();

    if (neg)
        mantissa *= -1;

    IOUAmount result (mantissa, exponent);

    if (hasRem)
    {
        if (roundUp && !neg)
        {
            if (!result)
            {
                return IOUAmount (minMantissa, minExponent);
            }
            return IOUAmount (result.mantissa () + 1, result.exponent ());
        }

        if (!roundUp && neg)
        {
            if (!result)
            {
                return IOUAmount (-minMantissa, minExponent);
            }
            return IOUAmount (result.mantissa () - 1, result.exponent ());
        }
    }

    return result;
}


}
























