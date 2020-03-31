

#ifndef RIPPLE_APP_PATHS_CREDIT_H_INCLUDED
#define RIPPLE_APP_PATHS_CREDIT_H_INCLUDED

#include <ripple/ledger/View.h>
#include <ripple/protocol/STAmount.h>
#include <ripple/protocol/IOUAmount.h>

namespace ripple {



STAmount creditLimit (
    ReadView const& view,
    AccountID const& account,
    AccountID const& issuer,
    Currency const& currency);

IOUAmount
creditLimit2 (
    ReadView const& v,
    AccountID const& acc,
    AccountID const& iss,
    Currency const& cur);




STAmount creditBalance (
    ReadView const& view,
    AccountID const& account,
    AccountID const& issuer,
    Currency const& currency);


} 

#endif








