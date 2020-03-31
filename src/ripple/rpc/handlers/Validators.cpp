

#include <ripple/app/main/Application.h>
#include <ripple/app/misc/ValidatorList.h>
#include <ripple/rpc/Context.h>

namespace ripple {

Json::Value
doValidators(RPC::Context& context)
{
    return context.app.validators().getJson();
}

}  
























