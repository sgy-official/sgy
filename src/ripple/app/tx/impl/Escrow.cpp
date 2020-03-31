

#include <ripple/app/tx/impl/Escrow.h>

#include <ripple/app/misc/HashRouter.h>
#include <ripple/basics/chrono.h>
#include <ripple/basics/Log.h>
#include <ripple/basics/safe_cast.h>
#include <ripple/conditions/Condition.h>
#include <ripple/conditions/Fulfillment.h>
#include <ripple/ledger/ApplyView.h>
#include <ripple/ledger/View.h>
#include <ripple/protocol/digest.h>
#include <ripple/protocol/st.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/Indexes.h>
#include <ripple/protocol/TxFlags.h>
#include <ripple/protocol/XRPAmount.h>

#define SF_CF_INVALID  SF_PRIVATE5
#define SF_CF_VALID    SF_PRIVATE6

namespace ripple {





static inline bool after (NetClock::time_point now, std::uint32_t mark)
{
    return now.time_since_epoch().count() > mark;
}

XRPAmount
EscrowCreate::calculateMaxSpend(STTx const& tx)
{
    return tx[sfAmount].xrp();
}

NotTEC
EscrowCreate::preflight (PreflightContext const& ctx)
{
    if (! ctx.rules.enabled(featureEscrow))
        return temDISABLED;

    if (ctx.rules.enabled(fix1543) && ctx.tx.getFlags() & tfUniversalMask)
        return temINVALID_FLAG;

    auto const ret = preflight1 (ctx);
    if (!isTesSuccess (ret))
        return ret;

    if (! isXRP(ctx.tx[sfAmount]))
        return temBAD_AMOUNT;

    if (ctx.tx[sfAmount] <= beast::zero)
        return temBAD_AMOUNT;

    if (! ctx.tx[~sfCancelAfter] && ! ctx.tx[~sfFinishAfter])
            return temBAD_EXPIRATION;

    if (ctx.tx[~sfCancelAfter] && ctx.tx[~sfFinishAfter] &&
            ctx.tx[sfCancelAfter] <= ctx.tx[sfFinishAfter])
        return temBAD_EXPIRATION;

    if (ctx.rules.enabled(fix1571))
    {
        if (! ctx.tx[~sfFinishAfter] && ! ctx.tx[~sfCondition])
            return temMALFORMED;
    }

    if (auto const cb = ctx.tx[~sfCondition])
    {
        using namespace ripple::cryptoconditions;

        std::error_code ec;

        auto condition = Condition::deserialize(*cb, ec);
        if (!condition)
        {
            JLOG(ctx.j.debug()) <<
                "Malformed condition during escrow creation: " << ec.message();
            return temMALFORMED;
        }

        if (condition->type != Type::preimageSha256 &&
                !ctx.rules.enabled(featureCryptoConditionsSuite))
            return temDISABLED;
    }

    return preflight2 (ctx);
}

TER
EscrowCreate::doApply()
{
    auto const closeTime = ctx_.view ().info ().parentCloseTime;

    if (ctx_.view ().rules().enabled(fix1571))
    {
        if (ctx_.tx[~sfCancelAfter] && after(closeTime, ctx_.tx[sfCancelAfter]))
            return tecNO_PERMISSION;

        if (ctx_.tx[~sfFinishAfter] && after(closeTime, ctx_.tx[sfFinishAfter]))
            return tecNO_PERMISSION;
    }
    else
    {
        if (ctx_.tx[~sfCancelAfter])
        {
            auto const cancelAfter = ctx_.tx[sfCancelAfter];

            if (closeTime.time_since_epoch().count() >= cancelAfter)
                return tecNO_PERMISSION;
        }

        if (ctx_.tx[~sfFinishAfter])
        {
            auto const finishAfter = ctx_.tx[sfFinishAfter];

            if (closeTime.time_since_epoch().count() >= finishAfter)
                return tecNO_PERMISSION;
        }
    }

    auto const account = ctx_.tx[sfAccount];
    auto const sle = ctx_.view().peek(
        keylet::account(account));

    {
        auto const balance = STAmount((*sle)[sfBalance]).xrp();
        auto const reserve = ctx_.view().fees().accountReserve(
            (*sle)[sfOwnerCount] + 1);

        if (balance < reserve)
            return tecINSUFFICIENT_RESERVE;

        if (balance < reserve + STAmount(ctx_.tx[sfAmount]).xrp())
            return tecUNFUNDED;
    }

    {
        auto const sled = ctx_.view().read(
            keylet::account(ctx_.tx[sfDestination]));
        if (! sled)
            return tecNO_DST;
        if (((*sled)[sfFlags] & lsfRequireDestTag) &&
                ! ctx_.tx[~sfDestinationTag])
            return tecDST_TAG_NEEDED;

        if (! ctx_.view().rules().enabled(featureDepositAuth) &&
                ((*sled)[sfFlags] & lsfDisallowXRP))
            return tecNO_TARGET;
    }

    auto const slep = std::make_shared<SLE>(
        keylet::escrow(account, (*sle)[sfSequence] - 1));
    (*slep)[sfAmount] = ctx_.tx[sfAmount];
    (*slep)[sfAccount] = account;
    (*slep)[~sfCondition] = ctx_.tx[~sfCondition];
    (*slep)[~sfSourceTag] = ctx_.tx[~sfSourceTag];
    (*slep)[sfDestination] = ctx_.tx[sfDestination];
    (*slep)[~sfCancelAfter] = ctx_.tx[~sfCancelAfter];
    (*slep)[~sfFinishAfter] = ctx_.tx[~sfFinishAfter];
    (*slep)[~sfDestinationTag] = ctx_.tx[~sfDestinationTag];

    ctx_.view().insert(slep);

    {
        auto page = dirAdd(ctx_.view(), keylet::ownerDir(account), slep->key(),
            false, describeOwnerDir(account), ctx_.app.journal ("View"));
        if (!page)
            return tecDIR_FULL;
        (*slep)[sfOwnerNode] = *page;
    }

    if (ctx_.view ().rules().enabled(fix1523))
    {
        auto const dest = ctx_.tx[sfDestination];

        if (dest != ctx_.tx[sfAccount])
        {
            auto page = dirAdd(ctx_.view(), keylet::ownerDir(dest), slep->key(),
                false, describeOwnerDir(dest), ctx_.app.journal ("View"));
            if (!page)
                return tecDIR_FULL;
            (*slep)[sfDestinationNode] = *page;
        }
    }

    (*sle)[sfBalance] = (*sle)[sfBalance] - ctx_.tx[sfAmount];
    adjustOwnerCount(ctx_.view(), sle, 1, ctx_.journal);
    ctx_.view().update(sle);

    return tesSUCCESS;
}


static
bool
checkCondition (Slice f, Slice c)
{
    using namespace ripple::cryptoconditions;

    std::error_code ec;

    auto condition = Condition::deserialize(c, ec);
    if (!condition)
        return false;

    auto fulfillment = Fulfillment::deserialize(f, ec);
    if (!fulfillment)
        return false;

    return validate (*fulfillment, *condition);
}

NotTEC
EscrowFinish::preflight (PreflightContext const& ctx)
{
    if (! ctx.rules.enabled(featureEscrow))
        return temDISABLED;

    if (ctx.rules.enabled(fix1543) && ctx.tx.getFlags() & tfUniversalMask)
        return temINVALID_FLAG;

    {
        auto const ret = preflight1 (ctx);
        if (!isTesSuccess (ret))
            return ret;
    }

    auto const cb = ctx.tx[~sfCondition];
    auto const fb = ctx.tx[~sfFulfillment];

    if (static_cast<bool>(cb) != static_cast<bool>(fb))
        return temMALFORMED;

    {
        auto const ret = preflight2 (ctx);
        if (!isTesSuccess (ret))
            return ret;
    }

    if (cb && fb)
    {
        auto& router = ctx.app.getHashRouter();

        auto const id = ctx.tx.getTransactionID();
        auto const flags = router.getFlags (id);

        if (!(flags & (SF_CF_INVALID | SF_CF_VALID)))
        {
            if (checkCondition (*fb, *cb))
                router.setFlags (id, SF_CF_VALID);
            else
                router.setFlags (id, SF_CF_INVALID);
        }
    }

    return tesSUCCESS;
}

std::uint64_t
EscrowFinish::calculateBaseFee (
    ReadView const& view,
    STTx const& tx)
{
    std::uint64_t extraFee = 0;

    if (auto const fb = tx[~sfFulfillment])
    {
        extraFee += view.fees().units *
            (32 + safe_cast<std::uint64_t> (fb->size() / 16));
    }

    return Transactor::calculateBaseFee (view, tx) + extraFee;
}

TER
EscrowFinish::doApply()
{
    auto const k = keylet::escrow(
        ctx_.tx[sfOwner], ctx_.tx[sfOfferSequence]);
    auto const slep = ctx_.view().peek(k);
    if (! slep)
        return tecNO_TARGET;

    if (ctx_.view ().rules().enabled(fix1571))
    {
        auto const now = ctx_.view().info().parentCloseTime;

        if ((*slep)[~sfFinishAfter] && ! after(now, (*slep)[sfFinishAfter]))
            return tecNO_PERMISSION;

        if ((*slep)[~sfCancelAfter] && after(now, (*slep)[sfCancelAfter]))
            return tecNO_PERMISSION;
    }
    else
    {
        if ((*slep)[~sfFinishAfter] &&
            ctx_.view().info().parentCloseTime.time_since_epoch().count() <=
            (*slep)[sfFinishAfter])
            return tecNO_PERMISSION;

        if ((*slep)[~sfCancelAfter] &&
            ctx_.view().info().parentCloseTime.time_since_epoch().count() <=
            (*slep)[sfCancelAfter])
            return tecNO_PERMISSION;
    }

    {
        auto const id = ctx_.tx.getTransactionID();
        auto flags = ctx_.app.getHashRouter().getFlags (id);

        auto const cb = ctx_.tx[~sfCondition];

        if (cb && ! (flags & (SF_CF_INVALID | SF_CF_VALID)))
        {
            auto const fb = ctx_.tx[~sfFulfillment];

            if (!fb)
                return tecINTERNAL;

            if (checkCondition (*fb, *cb))
                flags = SF_CF_VALID;
            else
                flags = SF_CF_INVALID;

            ctx_.app.getHashRouter().setFlags (id, flags);
        }

        if (flags & SF_CF_INVALID)
            return tecCRYPTOCONDITION_ERROR;

        auto const cond = (*slep)[~sfCondition];

        if (!cond && cb)
            return tecCRYPTOCONDITION_ERROR;

        if (cond && (cond != cb))
            return tecCRYPTOCONDITION_ERROR;
    }

    AccountID const destID = (*slep)[sfDestination];
    auto const sled = ctx_.view().peek(keylet::account(destID));
    if (! sled)
        return tecNO_DST;

    if (ctx_.view().rules().enabled(featureDepositAuth))
    {
        if (sled->getFlags() & lsfDepositAuth)
        {
            if (account_ != destID)
            {
                if (! view().exists (keylet::depositPreauth (destID, account_)))
                    return tecNO_PERMISSION;
            }
        }
    }

    AccountID const account = (*slep)[sfAccount];

    {
        auto const page = (*slep)[sfOwnerNode];
        if (! ctx_.view().dirRemove(
                keylet::ownerDir(account), page, k.key, true))
        {
            return tefBAD_LEDGER;
        }
    }

    if (ctx_.view ().rules().enabled(fix1523) && (*slep)[~sfDestinationNode])
    {
        auto const page = (*slep)[sfDestinationNode];
        if (! ctx_.view().dirRemove(keylet::ownerDir(destID), page, k.key, true))
        {
            return tefBAD_LEDGER;
        }
    }

    (*sled)[sfBalance] = (*sled)[sfBalance] + (*slep)[sfAmount];
    ctx_.view().update(sled);

    auto const sle = ctx_.view().peek(
        keylet::account(account));
    adjustOwnerCount(ctx_.view(), sle, -1, ctx_.journal);
    ctx_.view().update(sle);

    ctx_.view().erase(slep);

    return tesSUCCESS;
}


NotTEC
EscrowCancel::preflight (PreflightContext const& ctx)
{
    if (! ctx.rules.enabled(featureEscrow))
        return temDISABLED;

    if (ctx.rules.enabled(fix1543) && ctx.tx.getFlags() & tfUniversalMask)
        return temINVALID_FLAG;

    auto const ret = preflight1 (ctx);
    if (!isTesSuccess (ret))
        return ret;

    return preflight2 (ctx);
}

TER
EscrowCancel::doApply()
{
    auto const k = keylet::escrow(ctx_.tx[sfOwner], ctx_.tx[sfOfferSequence]);
    auto const slep = ctx_.view().peek(k);
    if (! slep)
        return tecNO_TARGET;

    if (ctx_.view ().rules().enabled(fix1571))
    {
        auto const now = ctx_.view().info().parentCloseTime;

        if (! (*slep)[~sfCancelAfter])
            return tecNO_PERMISSION;

        if (! after(now, (*slep)[sfCancelAfter]))
            return tecNO_PERMISSION;
    }
    else
    {
        if (!(*slep)[~sfCancelAfter] ||
            ctx_.view().info().parentCloseTime.time_since_epoch().count() <=
            (*slep)[sfCancelAfter])
            return tecNO_PERMISSION;
    }

    AccountID const account = (*slep)[sfAccount];

    {
        auto const page = (*slep)[sfOwnerNode];
        if (! ctx_.view().dirRemove(
                keylet::ownerDir(account), page, k.key, true))
        {
            return tefBAD_LEDGER;
        }
    }

    if (ctx_.view ().rules().enabled(fix1523) && (*slep)[~sfDestinationNode])
    {
        auto const page = (*slep)[sfDestinationNode];
        if (! ctx_.view().dirRemove(
                keylet::ownerDir((*slep)[sfDestination]), page, k.key, true))
        {
            return tefBAD_LEDGER;
        }
    }

    auto const sle = ctx_.view().peek(
        keylet::account(account));
    (*sle)[sfBalance] = (*sle)[sfBalance] + (*slep)[sfAmount];
    adjustOwnerCount(ctx_.view(), sle, -1, ctx_.journal);
    ctx_.view().update(sle);

    ctx_.view().erase(slep);

    return tesSUCCESS;
}

} 

























