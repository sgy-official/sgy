

#include <ripple/app/tx/impl/SetSignerList.h>

#include <ripple/app/ledger/Ledger.h>
#include <ripple/basics/Log.h>
#include <ripple/ledger/ApplyView.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/Indexes.h>
#include <ripple/protocol/STArray.h>
#include <ripple/protocol/STObject.h>
#include <ripple/protocol/STTx.h>
#include <algorithm>
#include <cstdint>

namespace ripple {

static std::uint32_t const defaultSignerListID_ = 0;

std::tuple<NotTEC, std::uint32_t,
    std::vector<SignerEntries::SignerEntry>,
        SetSignerList::Operation>
SetSignerList::determineOperation(STTx const& tx,
    ApplyFlags flags, beast::Journal j)
{
    auto const quorum = tx[sfSignerQuorum];
    std::vector<SignerEntries::SignerEntry> sign;
    Operation op = unknown;

    bool const hasSignerEntries(tx.isFieldPresent(sfSignerEntries));
    if (quorum && hasSignerEntries)
    {
        auto signers = SignerEntries::deserialize(tx, j, "transaction");

        if (signers.second != tesSUCCESS)
            return std::make_tuple(signers.second, quorum, sign, op);

        std::sort(signers.first.begin(), signers.first.end());

        sign = std::move(signers.first);
        op = set;
    }
    else if ((quorum == 0) && !hasSignerEntries)
    {
        op = destroy;
    }

    return std::make_tuple(tesSUCCESS, quorum, sign, op);
}

NotTEC
SetSignerList::preflight (PreflightContext const& ctx)
{
    if (! ctx.rules.enabled(featureMultiSign))
        return temDISABLED;

    auto const ret = preflight1 (ctx);
    if (!isTesSuccess (ret))
        return ret;

    auto const result = determineOperation(ctx.tx, ctx.flags, ctx.j);
    if (std::get<0>(result) != tesSUCCESS)
        return std::get<0>(result);

    if (std::get<3>(result) == unknown)
    {
        JLOG(ctx.j.trace()) <<
            "Malformed transaction: Invalid signer set list format.";
        return temMALFORMED;
    }

    if (std::get<3>(result) == set)
    {
        auto const account = ctx.tx.getAccountID(sfAccount);
        NotTEC const ter =
            validateQuorumAndSignerEntries(std::get<1>(result),
                std::get<2>(result), account, ctx.j);
        if (ter != tesSUCCESS)
        {
            return ter;
        }
    }

    return preflight2 (ctx);
}

TER
SetSignerList::doApply ()
{
    switch (do_)
    {
    case set:
        return replaceSignerList ();

    case destroy:
        return destroySignerList ();

    default:
        break;
    }
    assert (false); 
    return temMALFORMED;
}

void
SetSignerList::preCompute()
{
    auto result = determineOperation(ctx_.tx, view().flags(), j_);
    assert(std::get<0>(result) == tesSUCCESS);
    assert(std::get<3>(result) != unknown);

    quorum_ = std::get<1>(result);
    signers_ = std::get<2>(result);
    do_ = std::get<3>(result);

    return Transactor::preCompute();
}

NotTEC
SetSignerList::validateQuorumAndSignerEntries (
    std::uint32_t quorum,
        std::vector<SignerEntries::SignerEntry> const& signers,
            AccountID const& account,
                beast::Journal j)
{
    {
        std::size_t const signerCount = signers.size ();
        if ((signerCount < STTx::minMultiSigners)
            || (signerCount > STTx::maxMultiSigners))
        {
            JLOG(j.trace()) << "Too many or too few signers in signer list.";
            return temMALFORMED;
        }
    }

    assert(std::is_sorted(signers.begin(), signers.end()));
    if (std::adjacent_find (
        signers.begin (), signers.end ()) != signers.end ())
    {
        JLOG(j.trace()) << "Duplicate signers in signer list";
        return temBAD_SIGNER;
    }

    std::uint64_t allSignersWeight (0);
    for (auto const& signer : signers)
    {
        std::uint32_t const weight = signer.weight;
        if (weight <= 0)
        {
            JLOG(j.trace()) << "Every signer must have a positive weight.";
            return temBAD_WEIGHT;
        }

        allSignersWeight += signer.weight;

        if (signer.account == account)
        {
            JLOG(j.trace()) << "A signer may not self reference account.";
            return temBAD_SIGNER;
        }

    }
    if ((quorum <= 0) || (allSignersWeight < quorum))
    {
        JLOG(j.trace()) << "Quorum is unreachable";
        return temBAD_QUORUM;
    }
    return tesSUCCESS;
}

TER
SetSignerList::replaceSignerList ()
{
    auto const accountKeylet = keylet::account (account_);
    auto const ownerDirKeylet = keylet::ownerDir (account_);
    auto const signerListKeylet = keylet::signers (account_);

    if (TER const ter = removeSignersFromLedger (
        accountKeylet, ownerDirKeylet, signerListKeylet))
            return ter;

    auto const sle = view().peek(accountKeylet);

    std::uint32_t const oldOwnerCount {(*sle)[sfOwnerCount]};

    int addedOwnerCount {1};
    std::uint32_t flags {lsfOneOwnerCount};
    if (! ctx_.view().rules().enabled(featureMultiSignReserve))
    {
        addedOwnerCount = signerCountBasedOwnerCountDelta (signers_.size ());
        flags = 0;
    }

    XRPAmount const newReserve {
        view().fees().accountReserve(oldOwnerCount + addedOwnerCount)};

    if (mPriorBalance < newReserve)
        return tecINSUFFICIENT_RESERVE;

    auto signerList = std::make_shared<SLE>(signerListKeylet);
    view().insert (signerList);
    writeSignersToSLE (signerList, flags);

    auto viewJ = ctx_.app.journal ("View");
    auto const page = dirAdd (ctx_.view (), ownerDirKeylet,
        signerListKeylet.key, false, describeOwnerDir (account_), viewJ);

    JLOG(j_.trace()) << "Create signer list for account " <<
        toBase58 (account_) << ": " << (page ? "success" : "failure");

    if (!page)
        return tecDIR_FULL;

    signerList->setFieldU64 (sfOwnerNode, *page);

    adjustOwnerCount(view(), sle, addedOwnerCount, viewJ);
    return tesSUCCESS;
}

TER
SetSignerList::destroySignerList ()
{
    auto const accountKeylet = keylet::account (account_);
    SLE::pointer ledgerEntry = view().peek (accountKeylet);
    if ((ledgerEntry->isFlag (lsfDisableMaster)) &&
        (!ledgerEntry->isFieldPresent (sfRegularKey)))
            return tecNO_ALTERNATIVE_KEY;

    auto const ownerDirKeylet = keylet::ownerDir (account_);
    auto const signerListKeylet = keylet::signers (account_);
    return removeSignersFromLedger(
        accountKeylet, ownerDirKeylet, signerListKeylet);
}

TER
SetSignerList::removeSignersFromLedger (Keylet const& accountKeylet,
        Keylet const& ownerDirKeylet, Keylet const& signerListKeylet)
{
    SLE::pointer signers = view().peek (signerListKeylet);

    if (!signers)
        return tesSUCCESS;

    int removeFromOwnerCount = -1;
    if ((signers->getFlags() & lsfOneOwnerCount) == 0)
    {
        STArray const& actualList = signers->getFieldArray (sfSignerEntries);
        removeFromOwnerCount =
            signerCountBasedOwnerCountDelta (actualList.size()) * -1;
    }

    auto const hint = (*signers)[sfOwnerNode];

    if (! ctx_.view().dirRemove(
            ownerDirKeylet, hint, signerListKeylet.key, false))
    {
        return tefBAD_LEDGER;
    }

    auto viewJ = ctx_.app.journal("View");
    adjustOwnerCount(
        view(), view().peek(accountKeylet), removeFromOwnerCount, viewJ);

    ctx_.view().erase (signers);

    return tesSUCCESS;
}

void
SetSignerList::writeSignersToSLE (
    SLE::pointer const& ledgerEntry, std::uint32_t flags) const
{
    ledgerEntry->setFieldU32 (sfSignerQuorum, quorum_);
    ledgerEntry->setFieldU32 (sfSignerListID, defaultSignerListID_);
    if (flags) 
        ledgerEntry->setFieldU32 (sfFlags, flags);

    STArray toLedger (signers_.size ());
    for (auto const& entry : signers_)
    {
        toLedger.emplace_back(sfSignerEntry);
        STObject& obj = toLedger.back();
        obj.reserve (2);
        obj.setAccountID (sfAccount, entry.account);
        obj.setFieldU16 (sfSignerWeight, entry.weight);
    }

    ledgerEntry->setFieldArray (sfSignerEntries, toLedger);
}

int
SetSignerList::signerCountBasedOwnerCountDelta (std::size_t entryCount)
{
    assert (entryCount >= STTx::minMultiSigners);
    assert (entryCount <= STTx::maxMultiSigners);
    return 2 + static_cast<int>(entryCount);
}

} 
























