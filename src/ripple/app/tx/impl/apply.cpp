

#include <ripple/basics/Log.h>
#include <ripple/app/tx/apply.h>
#include <ripple/app/tx/applySteps.h>
#include <ripple/app/misc/HashRouter.h>
#include <ripple/protocol/Feature.h>

namespace ripple {

#define SF_SIGBAD      SF_PRIVATE1    
#define SF_SIGGOOD     SF_PRIVATE2    
#define SF_LOCALBAD    SF_PRIVATE3    
#define SF_LOCALGOOD   SF_PRIVATE4    


std::pair<Validity, std::string>
checkValidity(HashRouter& router,
    STTx const& tx, Rules const& rules,
        Config const& config)
{
    auto const allowMultiSign =
        rules.enabled(featureMultiSign);

    auto const id = tx.getTransactionID();
    auto const flags = router.getFlags(id);
    if (flags & SF_SIGBAD)
        return {Validity::SigBad, "Transaction has bad signature."};

    if (!(flags & SF_SIGGOOD))
    {
        auto const sigVerify = tx.checkSign(allowMultiSign);
        if (! sigVerify.first)
        {
            router.setFlags(id, SF_SIGBAD);
            return {Validity::SigBad, sigVerify.second};
        }
        router.setFlags(id, SF_SIGGOOD);
    }

    if (flags & SF_LOCALBAD)
        return {Validity::SigGoodOnly, "Local checks failed."};

    if (flags & SF_LOCALGOOD)
        return {Validity::Valid, ""};

    std::string reason;
    if (!passesLocalChecks(tx, reason))
    {
        router.setFlags(id, SF_LOCALBAD);
        return {Validity::SigGoodOnly, reason};
    }
    router.setFlags(id, SF_LOCALGOOD);
    return {Validity::Valid, ""};
}

void
forceValidity(HashRouter& router, uint256 const& txid,
    Validity validity)
{
    int flags = 0;
    switch (validity)
    {
    case Validity::Valid:
        flags |= SF_LOCALGOOD;
    case Validity::SigGoodOnly:
        flags |= SF_SIGGOOD;
    case Validity::SigBad:
        break;
    }
    if (flags)
        router.setFlags(txid, flags);
}

std::pair<TER, bool>
apply (Application& app, OpenView& view,
    STTx const& tx, ApplyFlags flags,
        beast::Journal j)
{
    STAmountSO saved(view.info().parentCloseTime);
    auto pfresult = preflight(app, view.rules(), tx, flags, j);
    auto pcresult = preclaim(pfresult, app, view);
    return doApply(pcresult, app, view);
}

ApplyResult
applyTransaction (Application& app, OpenView& view,
    STTx const& txn,
        bool retryAssured, ApplyFlags flags,
            beast::Journal j)
{
    if (retryAssured)
        flags = flags | tapRETRY;

    JLOG (j.debug()) << "TXN " << txn.getTransactionID ()
        << (retryAssured ? "/retry" : "/final");

    try
    {
        auto const result = apply(app, view, txn, flags, j);
        if (result.second)
        {
            JLOG (j.debug())
                << "Transaction applied: " << transHuman (result.first);
            return ApplyResult::Success;
        }

        if (isTefFailure (result.first) || isTemMalformed (result.first) ||
            isTelLocal (result.first))
        {
            JLOG (j.debug())
                << "Transaction failure: " << transHuman (result.first);
            return ApplyResult::Fail;
        }

        JLOG (j.debug())
            << "Transaction retry: " << transHuman (result.first);
        return ApplyResult::Retry;
    }
    catch (std::exception const&)
    {
        JLOG (j.warn()) << "Throws";
        return ApplyResult::Fail;
    }
}

} 
























