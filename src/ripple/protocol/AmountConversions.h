

#ifndef RIPPLE_PROTOCOL_AMOUNTCONVERSION_H_INCLUDED
#define RIPPLE_PROTOCOL_AMOUNTCONVERSION_H_INCLUDED

#include <ripple/protocol/IOUAmount.h>
#include <ripple/protocol/XRPAmount.h>
#include <ripple/protocol/STAmount.h>

namespace ripple {

inline
STAmount
toSTAmount (IOUAmount const& iou, Issue const& iss)
{
    bool const isNeg = iou.signum() < 0;
    std::uint64_t const umant = isNeg ? - iou.mantissa () : iou.mantissa ();
    return STAmount (iss, umant, iou.exponent (),  false, isNeg,
                     STAmount::unchecked ());
}

inline
STAmount
toSTAmount (IOUAmount const& iou)
{
    return toSTAmount (iou, noIssue ());
}

inline
STAmount
toSTAmount (XRPAmount const& xrp)
{
    bool const isNeg = xrp.signum() < 0;
    std::uint64_t const umant = isNeg ? - xrp.drops () : xrp.drops ();
    return STAmount (umant, isNeg);
}

inline
STAmount
toSTAmount (XRPAmount const& xrp, Issue const& iss)
{
    assert (isXRP(iss.account) && isXRP(iss.currency));
    return toSTAmount (xrp);
}

template <class T>
T
toAmount (STAmount const& amt)
{
    static_assert(sizeof(T) == -1, "Must use specialized function");
    return T(0);
}

template <>
inline
STAmount
toAmount<STAmount> (STAmount const& amt)
{
    return amt;
}

template <>
inline
IOUAmount
toAmount<IOUAmount> (STAmount const& amt)
{
    assert (amt.mantissa () < std::numeric_limits<std::int64_t>::max ());
    bool const isNeg = amt.negative ();
    std::int64_t const sMant =
            isNeg ? - std::int64_t (amt.mantissa ()) : amt.mantissa ();

    assert (! isXRP (amt));
    return IOUAmount (sMant, amt.exponent ());
}

template <>
inline
XRPAmount
toAmount<XRPAmount> (STAmount const& amt)
{
    assert (amt.mantissa () < std::numeric_limits<std::int64_t>::max ());
    bool const isNeg = amt.negative ();
    std::int64_t const sMant =
            isNeg ? - std::int64_t (amt.mantissa ()) : amt.mantissa ();

    assert (isXRP (amt));
    return XRPAmount (sMant);
}


}

#endif








