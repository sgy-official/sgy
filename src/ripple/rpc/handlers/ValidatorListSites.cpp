

#include <ripple/app/main/Application.h>
#include <ripple/app/misc/ValidatorSite.h>
#include <ripple/rpc/Context.h>

namespace ripple {

Json::Value
doValidatorListSites(RPC::Context& context)
{
    return context.app.validatorSites().getJson();
}

}  
























