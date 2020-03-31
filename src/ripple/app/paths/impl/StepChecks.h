

#ifndef RIPPLE_APP_PATHS_IMPL_STEP_CHECKS_H_INCLUDED
#define RIPPLE_APP_PATHS_IMPL_STEP_CHECKS_H_INCLUDED

#include <ripple/basics/Log.h>
#include <ripple/beast/utility/Journal.h>
#include <ripple/ledger/ReadView.h>
#include <ripple/protocol/AccountID.h>
#include <ripple/protocol/UintTypes.h>

namespace ripple {

inline
TER
checkFreeze (
    ReadView const& view,
    AccountID const& src,
    AccountID const& dst,
    Currency const& currency)
{
    assert (src != dst);

    if (auto sle = view.read (keylet::account (dst)))
    {
        if (sle->isFlag (lsfGlobalFreeze))
        {
            return terNO_LINE;
        }
    }

    if (auto sle = view.read (keylet::line (src, dst, currency)))
    {
        if (sle->isFlag ((dst > src) ? lsfHighFreeze : lsfLowFreeze))
        {
            return terNO_LINE;
        }
    }

    return tesSUCCESS;
}

inline
TER
checkNoRipple (
    ReadView const& view,
    AccountID const& prev,
    AccountID const& cur,
    AccountID const& next,
    Currency const& currency,
    beast::Journal j)
{
    auto sleIn = view.read (keylet::line (prev, cur, currency));
    auto sleOut = view.read (keylet::line (cur, next, currency));

    if (!sleIn || !sleOut)
        return terNO_LINE;

    if ((*sleIn)[sfFlags] &
            ((cur > prev) ? lsfHighNoRipple : lsfLowNoRipple) &&
        (*sleOut)[sfFlags] &
            ((cur > next) ? lsfHighNoRipple : lsfLowNoRipple))
    {
        JLOG (j.info()) << "Path violates noRipple constraint between " << prev
                      << ", " << cur << " and " << next;

        return terNO_RIPPLE;
    }
    return tesSUCCESS;
}

}

#endif








