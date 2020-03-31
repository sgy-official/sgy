

#include <ripple/app/paths/cursor/RippleLiquidity.h>
#include <ripple/basics/contract.h>
#include <ripple/basics/Log.h>

namespace ripple {
namespace path {


void PathCursor::nextIncrement () const
{

    auto status = liquidity();

    if (status == tesSUCCESS)
    {
        if (pathState_.isDry())
        {
            JLOG (j_.debug())
                << "nextIncrement: success on dry path:"
                << " outPass=" << pathState_.outPass()
                << " inPass=" << pathState_.inPass();
            Throw<std::runtime_error> ("Made no progress.");
        }

        pathState_.setQuality(getRate (
            pathState_.outPass(), pathState_.inPass()));
    }
    else
    {
        pathState_.setQuality(0);
    }
    pathState_.setStatus (status);
}

} 
} 
























