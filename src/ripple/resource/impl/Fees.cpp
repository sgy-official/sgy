

#include <ripple/resource/Fees.h>

namespace ripple {
namespace Resource {

Charge const feeInvalidRequest    (  100, "malformed request"      );
Charge const feeRequestNoReply    (   10, "unsatisfiable request"  );
Charge const feeInvalidSignature  ( 1000, "invalid signature"      );
Charge const feeUnwantedData      (  150, "useless data"           );
Charge const feeBadData           (  200, "invalid data"           );

Charge const feeInvalidRPC        (  100, "malformed RPC"          );
Charge const feeReferenceRPC      (   20, "reference RPC"          );
Charge const feeExceptionRPC      (  100, "exceptioned RPC"        );
Charge const feeMediumBurdenRPC   (  400, "medium RPC"             );
Charge const feeHighBurdenRPC     ( 3000, "heavy RPC"              );

Charge const feeLightPeer         (    1, "trivial peer request"   );
Charge const feeMediumBurdenPeer  (  250, "moderate peer request"  );
Charge const feeHighBurdenPeer    ( 2000, "heavy peer request"     );

Charge const feeWarning           ( 2000, "received warning"       );
Charge const feeDrop              ( 3000, "dropped"                );

}
}
























