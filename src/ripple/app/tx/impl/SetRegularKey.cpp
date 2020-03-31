

#include <ripple/app/tx/impl/SetRegularKey.h>
#include <ripple/basics/Log.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/TxFlags.h>

namespace ripple {

std::uint64_t
SetRegularKey::calculateBaseFee (
    ReadView const& view,
    STTx const& tx)
{
    auto const id = tx.getAccountID(sfAccount);
    auto const spk = tx.getSigningPubKey();

    if (publicKeyType (makeSlice (spk)))
    {
        if (calcAccountID(PublicKey (makeSlice(spk))) == id)
        {
            auto const sle = view.read(keylet::account(id));

            if (sle && (! (sle->getFlags () & lsfPasswordSpent)))
            {
                return 0;
            }
        }
    }

    return Transactor::calculateBaseFee (view, tx);
}

NotTEC
SetRegularKey::preflight (PreflightContext const& ctx)
{
    auto const ret = preflight1 (ctx);
    if (!isTesSuccess (ret))
        return ret;

    std::uint32_t const uTxFlags = ctx.tx.getFlags ();

    if (uTxFlags & tfUniversalMask)
    {
        JLOG(ctx.j.trace()) <<
            "Malformed transaction: Invalid flags set.";

        return temINVALID_FLAG;
    }


    if (ctx.rules.enabled(fixMasterKeyAsRegularKey)
        && ctx.tx.isFieldPresent(sfRegularKey)
        && (ctx.tx.getAccountID(sfRegularKey) == ctx.tx.getAccountID(sfAccount)))
    {
        return temBAD_REGKEY;
    }

    return preflight2(ctx);
}

TER
SetRegularKey::doApply ()
{
    auto const sle = view ().peek (
        keylet::account (account_));

    if (!minimumFee (ctx_.app, ctx_.baseFee, view ().fees (), view ().flags ()))
        sle->setFlag (lsfPasswordSpent);

    if (ctx_.tx.isFieldPresent (sfRegularKey))
    {
        sle->setAccountID (sfRegularKey,
            ctx_.tx.getAccountID (sfRegularKey));
    }
    else
    {
        if (sle->isFlag (lsfDisableMaster) &&
            !view().peek (keylet::signers (account_)))
            return tecNO_ALTERNATIVE_KEY;

        sle->makeFieldAbsent (sfRegularKey);
    }

    return tesSUCCESS;
}

}
























