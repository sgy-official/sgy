

#ifndef RIPPLE_APP_PATHS_FLOW_H_INCLUDED
#define RIPPLE_APP_PATHS_FLOW_H_INCLUDED

#include <ripple/app/paths/impl/Steps.h>
#include <ripple/app/paths/RippleCalc.h>
#include <ripple/protocol/Quality.h>

namespace ripple
{

namespace path {
namespace detail{
struct FlowDebugInfo;
}
}


path::RippleCalc::Output
flow (PaymentSandbox& view,
    STAmount const& deliver,
    AccountID const& src,
    AccountID const& dst,
    STPathSet const& paths,
    bool defaultPaths,
    bool partialPayment,
    bool ownerPaysTransferFee,
    bool offerCrossing,
    boost::optional<Quality> const& limitQuality,
    boost::optional<STAmount> const& sendMax,
    beast::Journal j,
    path::detail::FlowDebugInfo* flowDebugInfo=nullptr);

}  

#endif








