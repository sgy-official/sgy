

#include <ripple/app/tx/impl/PayChan.h>
#include <ripple/basics/chrono.h>
#include <ripple/basics/Log.h>
#include <ripple/ledger/ApplyView.h>
#include <ripple/ledger/View.h>
#include <ripple/protocol/digest.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/Indexes.h>
#include <ripple/protocol/PayChan.h>
#include <ripple/protocol/PublicKey.h>
#include <ripple/protocol/st.h>
#include <ripple/protocol/TxFlags.h>
#include <ripple/protocol/XRPAmount.h>

namespace ripple {




static
TER
closeChannel (
    std::shared_ptr<SLE> const& slep,
    ApplyView& view,
    uint256 const& key,
    beast::Journal j)
{
    AccountID const src = (*slep)[sfAccount];
    {
        auto const page = (*slep)[sfOwnerNode];
        if (! view.dirRemove(keylet::ownerDir(src), page, key, true))
        {
            return tefBAD_LEDGER;
        }
    }

    auto const sle = view.peek (keylet::account (src));
    assert ((*slep)[sfAmount] >= (*slep)[sfBalance]);
    (*sle)[sfBalance] =
        (*sle)[sfBalance] + (*slep)[sfAmount] - (*slep)[sfBalance];
    adjustOwnerCount(view, sle, -1, j);
    view.update (sle);

    view.erase (slep);
    return tesSUCCESS;
}


NotTEC
PayChanCreate::preflight (PreflightContext const& ctx)
{
    if (!ctx.rules.enabled (featurePayChan))
        return temDISABLED;

    if (ctx.rules.enabled(fix1543) && ctx.tx.getFlags() & tfUniversalMask)
        return temINVALID_FLAG;

    auto const ret = preflight1 (ctx);
    if (!isTesSuccess (ret))
        return ret;

    if (!isXRP (ctx.tx[sfAmount]) || (ctx.tx[sfAmount] <= beast::zero))
        return temBAD_AMOUNT;

    if (ctx.tx[sfAccount] == ctx.tx[sfDestination])
        return temDST_IS_SRC;

    if (!publicKeyType(ctx.tx[sfPublicKey]))
        return temMALFORMED;

    return preflight2 (ctx);
}

TER
PayChanCreate::preclaim(PreclaimContext const &ctx)
{
    auto const account = ctx.tx[sfAccount];
    auto const sle = ctx.view.read (keylet::account (account));

    {
        auto const balance = (*sle)[sfBalance];
        auto const reserve =
                ctx.view.fees ().accountReserve ((*sle)[sfOwnerCount] + 1);

        if (balance < reserve)
            return tecINSUFFICIENT_RESERVE;

        if (balance < reserve + ctx.tx[sfAmount])
            return tecUNFUNDED;
    }

    auto const dst = ctx.tx[sfDestination];

    {
        auto const sled = ctx.view.read (keylet::account (dst));
        if (!sled)
            return tecNO_DST;
        if (((*sled)[sfFlags] & lsfRequireDestTag) &&
            !ctx.tx[~sfDestinationTag])
            return tecDST_TAG_NEEDED;

        if (!ctx.view.rules().enabled(featureDepositAuth) &&
            ((*sled)[sfFlags] & lsfDisallowXRP))
            return tecNO_TARGET;
    }

    return tesSUCCESS;
}

TER
PayChanCreate::doApply()
{
    auto const account = ctx_.tx[sfAccount];
    auto const sle = ctx_.view ().peek (keylet::account (account));
    auto const dst = ctx_.tx[sfDestination];

    auto const slep = std::make_shared<SLE> (
        keylet::payChan (account, dst, (*sle)[sfSequence] - 1));
    (*slep)[sfAmount] = ctx_.tx[sfAmount];
    (*slep)[sfBalance] = ctx_.tx[sfAmount].zeroed ();
    (*slep)[sfAccount] = account;
    (*slep)[sfDestination] = dst;
    (*slep)[sfSettleDelay] = ctx_.tx[sfSettleDelay];
    (*slep)[sfPublicKey] = ctx_.tx[sfPublicKey];
    (*slep)[~sfCancelAfter] = ctx_.tx[~sfCancelAfter];
    (*slep)[~sfSourceTag] = ctx_.tx[~sfSourceTag];
    (*slep)[~sfDestinationTag] = ctx_.tx[~sfDestinationTag];

    ctx_.view ().insert (slep);

    {
        auto page = dirAdd (ctx_.view(), keylet::ownerDir(account), slep->key(),
            false, describeOwnerDir (account), ctx_.app.journal ("View"));
        if (!page)
            return tecDIR_FULL;
        (*slep)[sfOwnerNode] = *page;
    }

    (*sle)[sfBalance] = (*sle)[sfBalance] - ctx_.tx[sfAmount];
    adjustOwnerCount(ctx_.view(), sle, 1, ctx_.journal);
    ctx_.view ().update (sle);

    return tesSUCCESS;
}


NotTEC
PayChanFund::preflight (PreflightContext const& ctx)
{
    if (!ctx.rules.enabled (featurePayChan))
        return temDISABLED;

    if (ctx.rules.enabled(fix1543) && ctx.tx.getFlags() & tfUniversalMask)
        return temINVALID_FLAG;

    auto const ret = preflight1 (ctx);
    if (!isTesSuccess (ret))
        return ret;

    if (!isXRP (ctx.tx[sfAmount]) || (ctx.tx[sfAmount] <= beast::zero))
        return temBAD_AMOUNT;

    return preflight2 (ctx);
}

TER
PayChanFund::doApply()
{
    Keylet const k (ltPAYCHAN, ctx_.tx[sfPayChannel]);
    auto const slep = ctx_.view ().peek (k);
    if (!slep)
        return tecNO_ENTRY;

    AccountID const src = (*slep)[sfAccount];
    auto const txAccount = ctx_.tx[sfAccount];
    auto const expiration = (*slep)[~sfExpiration];

    {
        auto const cancelAfter = (*slep)[~sfCancelAfter];
        auto const closeTime =
            ctx_.view ().info ().parentCloseTime.time_since_epoch ().count ();
        if ((cancelAfter && closeTime >= *cancelAfter) ||
            (expiration && closeTime >= *expiration))
            return closeChannel (
                slep, ctx_.view (), k.key, ctx_.app.journal ("View"));
    }

    if (src != txAccount)
        return tecNO_PERMISSION;

    if (auto extend = ctx_.tx[~sfExpiration])
    {
        auto minExpiration =
            ctx_.view ().info ().parentCloseTime.time_since_epoch ().count () +
                (*slep)[sfSettleDelay];
        if (expiration && *expiration < minExpiration)
            minExpiration = *expiration;

        if (*extend < minExpiration)
            return temBAD_EXPIRATION;
        (*slep)[~sfExpiration] = *extend;
        ctx_.view ().update (slep);
    }

    auto const sle = ctx_.view ().peek (keylet::account (txAccount));

    {
        auto const balance = (*sle)[sfBalance];
        auto const reserve =
            ctx_.view ().fees ().accountReserve ((*sle)[sfOwnerCount]);

        if (balance < reserve)
            return tecINSUFFICIENT_RESERVE;

        if (balance < reserve + ctx_.tx[sfAmount])
            return tecUNFUNDED;
    }

    (*slep)[sfAmount] = (*slep)[sfAmount] + ctx_.tx[sfAmount];
    ctx_.view ().update (slep);

    (*sle)[sfBalance] = (*sle)[sfBalance] - ctx_.tx[sfAmount];
    ctx_.view ().update (sle);

    return tesSUCCESS;
}


NotTEC
PayChanClaim::preflight (PreflightContext const& ctx)
{
    if (! ctx.rules.enabled(featurePayChan))
        return temDISABLED;


    auto const ret = preflight1 (ctx);
    if (!isTesSuccess (ret))
        return ret;

    auto const bal = ctx.tx[~sfBalance];
    if (bal && (!isXRP (*bal) || *bal <= beast::zero))
        return temBAD_AMOUNT;

    auto const amt = ctx.tx[~sfAmount];
    if (amt && (!isXRP (*amt) || *amt <= beast::zero))
        return temBAD_AMOUNT;

    if (bal && amt && *bal > *amt)
        return temBAD_AMOUNT;

    {
        auto const flags = ctx.tx.getFlags();

        if (ctx.rules.enabled(fix1543) && (flags & tfPayChanClaimMask))
            return temINVALID_FLAG;

        if ((flags & tfClose) && (flags & tfRenew))
            return temMALFORMED;
    }

    if (auto const sig = ctx.tx[~sfSignature])
    {
        if (!(ctx.tx[~sfPublicKey] && bal))
            return temMALFORMED;


        auto const reqBalance = bal->xrp ();
        auto const authAmt = amt ? amt->xrp() : reqBalance;

        if (reqBalance > authAmt)
            return temBAD_AMOUNT;

        Keylet const k (ltPAYCHAN, ctx.tx[sfPayChannel]);
        if (!publicKeyType(ctx.tx[sfPublicKey]))
            return temMALFORMED;
        PublicKey const pk (ctx.tx[sfPublicKey]);
        Serializer msg;
        serializePayChanAuthorization (msg, k.key, authAmt);
        if (!verify (pk, msg.slice (), *sig,  true))
            return temBAD_SIGNATURE;
    }

    return preflight2 (ctx);
}

TER
PayChanClaim::doApply()
{
    Keylet const k (ltPAYCHAN, ctx_.tx[sfPayChannel]);
    auto const slep = ctx_.view ().peek (k);
    if (!slep)
        return tecNO_TARGET;

    AccountID const src = (*slep)[sfAccount];
    AccountID const dst = (*slep)[sfDestination];
    AccountID const txAccount = ctx_.tx[sfAccount];

    auto const curExpiration = (*slep)[~sfExpiration];
    {
        auto const cancelAfter = (*slep)[~sfCancelAfter];
        auto const closeTime =
            ctx_.view ().info ().parentCloseTime.time_since_epoch ().count ();
        if ((cancelAfter && closeTime >= *cancelAfter) ||
            (curExpiration && closeTime >= *curExpiration))
            return closeChannel (
                slep, ctx_.view (), k.key, ctx_.app.journal ("View"));
    }

    if (txAccount != src && txAccount != dst)
        return tecNO_PERMISSION;

    if (ctx_.tx[~sfBalance])
    {
        auto const chanBalance = slep->getFieldAmount (sfBalance).xrp ();
        auto const chanFunds = slep->getFieldAmount (sfAmount).xrp ();
        auto const reqBalance = ctx_.tx[sfBalance].xrp ();

        if (txAccount == dst && !ctx_.tx[~sfSignature])
            return temBAD_SIGNATURE;

        if (ctx_.tx[~sfSignature])
        {
            PublicKey const pk ((*slep)[sfPublicKey]);
            if (ctx_.tx[sfPublicKey] != pk)
                return temBAD_SIGNER;
        }

        if (reqBalance > chanFunds)
            return tecUNFUNDED_PAYMENT;

        if (reqBalance <= chanBalance)
            return tecUNFUNDED_PAYMENT;

        auto const sled = ctx_.view ().peek (keylet::account (dst));
        if (!sled)
            return terNO_ACCOUNT;

        bool const depositAuth {ctx_.view().rules().enabled(featureDepositAuth)};
        if (!depositAuth &&
            (txAccount == src && (sled->getFlags() & lsfDisallowXRP)))
            return tecNO_TARGET;

        if (depositAuth && (sled->getFlags() & lsfDepositAuth))
        {
            if (txAccount != dst)
            {
                if (! view().exists (keylet::depositPreauth (dst, txAccount)))
                    return tecNO_PERMISSION;
            }
        }

        (*slep)[sfBalance] = ctx_.tx[sfBalance];
        XRPAmount const reqDelta = reqBalance - chanBalance;
        assert (reqDelta >= beast::zero);
        (*sled)[sfBalance] = (*sled)[sfBalance] + reqDelta;
        ctx_.view ().update (sled);
        ctx_.view ().update (slep);
    }

    if (ctx_.tx.getFlags () & tfRenew)
    {
        if (src != txAccount)
            return tecNO_PERMISSION;
        (*slep)[~sfExpiration] = boost::none;
        ctx_.view ().update (slep);
    }

    if (ctx_.tx.getFlags () & tfClose)
    {
        if (dst == txAccount || (*slep)[sfBalance] == (*slep)[sfAmount])
            return closeChannel (
                slep, ctx_.view (), k.key, ctx_.app.journal ("View"));

        auto const settleExpiration =
            ctx_.view ().info ().parentCloseTime.time_since_epoch ().count () +
                (*slep)[sfSettleDelay];

        if (!curExpiration || *curExpiration > settleExpiration)
        {
            (*slep)[~sfExpiration] = settleExpiration;
            ctx_.view ().update (slep);
        }
    }

    return tesSUCCESS;
}

} 

























