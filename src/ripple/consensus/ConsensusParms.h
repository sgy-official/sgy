

#ifndef RIPPLE_CONSENSUS_CONSENSUS_PARMS_H_INCLUDED
#define RIPPLE_CONSENSUS_CONSENSUS_PARMS_H_INCLUDED

#include <chrono>
#include <cstddef>

namespace ripple {


struct ConsensusParms
{
    explicit ConsensusParms() = default;

    
    std::chrono::seconds validationVALID_WALL = std::chrono::minutes {5};

    
    std::chrono::seconds validationVALID_LOCAL = std::chrono::minutes {3};

    
    std::chrono::seconds validationVALID_EARLY = std::chrono::minutes {3};


    std::chrono::seconds proposeFRESHNESS = std::chrono::seconds {20};

    std::chrono::seconds proposeINTERVAL = std::chrono::seconds {12};



    std::size_t minCONSENSUS_PCT = 80;

    std::chrono::milliseconds ledgerIDLE_INTERVAL = std::chrono::seconds {15};

    std::chrono::milliseconds ledgerMIN_CONSENSUS =
        std::chrono::milliseconds {1950};

    
    std::chrono::milliseconds ledgerMAX_CONSENSUS =
        std::chrono::seconds {10};

    std::chrono::milliseconds ledgerMIN_CLOSE = std::chrono::seconds {2};

    std::chrono::milliseconds ledgerGRANULARITY = std::chrono::seconds {1};

    
    std::chrono::milliseconds avMIN_CONSENSUS_TIME = std::chrono::seconds {5};


    std::size_t avINIT_CONSENSUS_PCT = 50;

    std::size_t avMID_CONSENSUS_TIME = 50;

    std::size_t avMID_CONSENSUS_PCT = 65;

    std::size_t avLATE_CONSENSUS_TIME = 85;

    std::size_t avLATE_CONSENSUS_PCT = 70;

    std::size_t avSTUCK_CONSENSUS_TIME = 200;

    std::size_t avSTUCK_CONSENSUS_PCT = 95;

    std::size_t avCT_CONSENSUS_PCT = 75;


    
    bool useRoundedCloseTime = true;
};

}  
#endif








