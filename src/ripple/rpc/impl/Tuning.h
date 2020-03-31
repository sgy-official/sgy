

#ifndef RIPPLE_RPC_TUNING_H_INCLUDED
#define RIPPLE_RPC_TUNING_H_INCLUDED

namespace ripple {
namespace RPC {



namespace Tuning {


struct LimitRange {
    unsigned int rmin, rdefault, rmax;
};


static LimitRange const accountLines = {10, 200, 400};


static LimitRange const accountChannels = {10, 200, 400};


static LimitRange const accountObjects = {10, 200, 400};


static LimitRange const accountOffers = {10, 200, 400};


static LimitRange const bookOffers = {0, 300, 400};


static LimitRange const noRippleCheck = {10, 300, 400};

static int const defaultAutoFillFeeMultiplier = 10;
static int const defaultAutoFillFeeDivisor = 1;
static int const maxPathfindsInProgress = 2;
static int const maxPathfindJobCount = 50;
static int const maxJobQueueClients = 500;
auto constexpr maxValidatedLedgerAge = std::chrono::minutes {2};
static int const maxRequestSize = 1000000;


static int const binaryPageLength = 2048;


static int const jsonPageLength = 256;


inline int pageLength(bool isBinary)
{
    return isBinary ? binaryPageLength : jsonPageLength;
}


static int const max_src_cur = 18;


static int const max_auto_src_cur = 88;

} 


} 
} 

#endif








