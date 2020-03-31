

#ifndef RIPPLE_TX_APPLY_H_INCLUDED
#define RIPPLE_TX_APPLY_H_INCLUDED

#include <ripple/core/Config.h>
#include <ripple/ledger/View.h>
#include <ripple/protocol/STTx.h>
#include <ripple/protocol/TER.h>
#include <ripple/beast/utility/Journal.h>
#include <memory>
#include <utility>

namespace ripple {

class Application;
class HashRouter;


enum class Validity
{
    SigBad,
    SigGoodOnly,
    Valid
};


std::pair<Validity, std::string>
checkValidity(HashRouter& router,
    STTx const& tx, Rules const& rules,
        Config const& config);



void
forceValidity(HashRouter& router, uint256 const& txid,
    Validity validity);


std::pair<TER, bool>
apply (Application& app, OpenView& view,
    STTx const& tx, ApplyFlags flags,
        beast::Journal journal);



enum class ApplyResult
{
    Success,
    Fail,
    Retry
};


ApplyResult
applyTransaction(Application& app, OpenView& view,
    STTx const& tx, bool retryAssured, ApplyFlags flags,
    beast::Journal journal);

} 

#endif








