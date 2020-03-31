

#ifndef RIPPLE_APP_PATHS_CURSOR_RIPPLELIQUIDITY_H_INCLUDED
#define RIPPLE_APP_PATHS_CURSOR_RIPPLELIQUIDITY_H_INCLUDED

#include <ripple/app/paths/cursor/PathCursor.h>
#include <ripple/app/paths/RippleCalc.h>
#include <ripple/app/paths/Tuning.h>
#include <ripple/ledger/View.h>
#include <ripple/protocol/Rate.h>

namespace ripple {
namespace path {

void rippleLiquidity (
    RippleCalc&,
    Rate const& qualityIn,
    Rate const& qualityOut,
    STAmount const& saPrvReq,
    STAmount const& saCurReq,
    STAmount& saPrvAct,
    STAmount& saCurAct,
    std::uint64_t& uRateMax);

Rate
quality_in (
    ReadView const& view,
    AccountID const& uToAccountID,
    AccountID const& uFromAccountID,
    Currency const& currency);

Rate
quality_out (
    ReadView const& view,
    AccountID const& uToAccountID,
    AccountID const& uFromAccountID,
    Currency const& currency);

} 
} 

#endif








