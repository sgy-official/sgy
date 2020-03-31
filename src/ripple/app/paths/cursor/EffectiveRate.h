

#ifndef RIPPLE_APP_PATHS_CURSOR_EFFECTIVERATE_H_INCLUDED
#define RIPPLE_APP_PATHS_CURSOR_EFFECTIVERATE_H_INCLUDED

#include <ripple/protocol/AccountID.h>
#include <ripple/protocol/Issue.h>
#include <ripple/protocol/Rate.h>
#include <boost/optional.hpp>

namespace ripple {
namespace path {

Rate
effectiveRate(
    Issue const& issue,
    AccountID const& account1,
    AccountID const& account2,
    boost::optional<Rate> const& rate);

} 
} 

#endif








