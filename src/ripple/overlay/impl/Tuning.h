

#ifndef RIPPLE_OVERLAY_TUNING_H_INCLUDED
#define RIPPLE_OVERLAY_TUNING_H_INCLUDED

#include <chrono>

namespace ripple {

namespace Tuning
{

enum
{
    
    readBufferBytes     = 4096,

    
    maxInsaneTime       =   60,

    
    maxUnknownTime      =  300,

    
    saneLedgerLimit     =   24,

    
    insaneLedgerLimit   =  128,

    
    maxReplyNodes       = 8192,

    
    checkSeconds        =   32,

    
    timerSeconds        =    8,

    
    sendqIntervals      =    4,

    
    noPing              =   10,

    
    dropSendQueue       =   192,

    
    targetSendQueue     =   128,

    
    sendQueueLogFreq    =    64,
};


std::chrono::milliseconds constexpr peerHighLatency{300};

} 

} 

#endif








