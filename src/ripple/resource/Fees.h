

#ifndef RIPPLE_RESOURCE_FEES_H_INCLUDED
#define RIPPLE_RESOURCE_FEES_H_INCLUDED

#include <ripple/resource/Charge.h>

namespace ripple {
namespace Resource {



extern Charge const feeInvalidRequest;        
extern Charge const feeRequestNoReply;        
extern Charge const feeInvalidSignature;      
extern Charge const feeUnwantedData;          
extern Charge const feeBadData;               

extern Charge const feeInvalidRPC;            
extern Charge const feeReferenceRPC;          
extern Charge const feeExceptionRPC;          
extern Charge const feeLightRPC;              
extern Charge const feeLowBurdenRPC;          
extern Charge const feeMediumBurdenRPC;       
extern Charge const feeHighBurdenRPC;         
extern Charge const feePathFindUpdate;        

extern Charge const feeLightPeer;             
extern Charge const feeLowBurdenPeer;         
extern Charge const feeMediumBurdenPeer;      
extern Charge const feeHighBurdenPeer;        

extern Charge const feeNewTrustedNote;        
extern Charge const feeNewValidTx;            
extern Charge const feeSatisfiedRequest;      

extern Charge const feeWarning;               
extern Charge const feeDrop;                  


}
}

#endif








