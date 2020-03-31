

#include <ripple/app/tx/impl/InvariantCheck.h>
#include <ripple/basics/Log.h>

namespace ripple {

void
TransactionFeeCheck::visitEntry(
    uint256 const&,
    bool,
    std::shared_ptr<SLE const> const&,
    std::shared_ptr<SLE const> const&)
{
}

bool
TransactionFeeCheck::finalize(
    STTx const& tx,
    TER const result,
    XRPAmount const fee,
    beast::Journal const& j)
{
    if (fee.drops() < 0)
    {
        JLOG(j.fatal()) << "Invariant failed: fee paid was negative: " << fee.drops();
        return false;
    }

    if (fee.drops() >= SYSTEM_CURRENCY_START)
    {
        JLOG(j.fatal()) << "Invariant failed: fee paid exceeds system limit: " << fee.drops();
        return false;
    }

    if (fee > tx.getFieldAmount(sfFee).xrp())
    {
        JLOG(j.fatal()) << "Invariant failed: fee paid is " << fee.drops() <<
            " exceeds fee specified in transaction.";
        return false;
    }

    return true;
}


void
XRPNotCreated::visitEntry(
    uint256 const&,
    bool isDelete,
    std::shared_ptr <SLE const> const& before,
    std::shared_ptr <SLE const> const& after)
{
    
    if(before)
    {
        switch (before->getType())
        {
        case ltACCOUNT_ROOT:
            drops_ -= (*before)[sfBalance].xrp().drops();
            break;
        case ltPAYCHAN:
            drops_ -= ((*before)[sfAmount] - (*before)[sfBalance]).xrp().drops();
            break;
        case ltESCROW:
            drops_ -= (*before)[sfAmount].xrp().drops();
            break;
        default:
            break;
        }
    }

    if(after)
    {
        switch (after->getType())
        {
        case ltACCOUNT_ROOT:
            drops_ += (*after)[sfBalance].xrp().drops();
            break;
        case ltPAYCHAN:
            if (! isDelete)
                drops_ += ((*after)[sfAmount] - (*after)[sfBalance]).xrp().drops();
            break;
        case ltESCROW:
            if (! isDelete)
                drops_ += (*after)[sfAmount].xrp().drops();
            break;
        default:
            break;
        }
    }
}

bool
XRPNotCreated::finalize(
    STTx const& tx,
    TER const,
    XRPAmount const fee,
    beast::Journal const& j)
{
    if (drops_ > 0)
    {
        JLOG(j.fatal()) <<
            "Invariant failed: XRP net change was positive: " << drops_;
        return false;
    }

    if (-drops_ != fee.drops())
    {
        JLOG(j.fatal()) <<
            "Invariant failed: XRP net change of " << drops_ <<
            " doesn't match fee " << fee.drops();
        return false;
    }

    return true;
}


void
XRPBalanceChecks::visitEntry(
    uint256 const&,
    bool,
    std::shared_ptr <SLE const> const& before,
    std::shared_ptr <SLE const> const& after)
{
    auto isBad = [](STAmount const& balance)
    {
        if (!balance.native())
            return true;

        auto const drops = balance.xrp().drops();

        if (drops > SYSTEM_CURRENCY_START)
            return true;

        if (drops < 0)
            return true;

        return false;
    };

    if(before && before->getType() == ltACCOUNT_ROOT)
        bad_ |= isBad ((*before)[sfBalance]);

    if(after && after->getType() == ltACCOUNT_ROOT)
        bad_ |= isBad ((*after)[sfBalance]);
}

bool
XRPBalanceChecks::finalize(STTx const&, TER const, XRPAmount const, beast::Journal const& j)
{
    if (bad_)
    {
        JLOG(j.fatal()) << "Invariant failed: incorrect account XRP balance";
        return false;
    }

    return true;
}


void
NoBadOffers::visitEntry(
    uint256 const&,
    bool isDelete,
    std::shared_ptr <SLE const> const& before,
    std::shared_ptr <SLE const> const& after)
{
    auto isBad = [](STAmount const& pays, STAmount const& gets)
    {
        if (pays < beast::zero)
            return true;

        if (gets < beast::zero)
            return true;

        return pays.native() && gets.native();
    };

    if(before && before->getType() == ltOFFER)
        bad_ |= isBad ((*before)[sfTakerPays], (*before)[sfTakerGets]);

    if(after && after->getType() == ltOFFER)
        bad_ |= isBad((*after)[sfTakerPays], (*after)[sfTakerGets]);
}

bool
NoBadOffers::finalize(STTx const& tx, TER const, XRPAmount const, beast::Journal const& j)
{
    if (bad_)
    {
        JLOG(j.fatal()) << "Invariant failed: offer with a bad amount";
        return false;
    }

    return true;
}


void
NoZeroEscrow::visitEntry(
    uint256 const&,
    bool isDelete,
    std::shared_ptr <SLE const> const& before,
    std::shared_ptr <SLE const> const& after)
{
    auto isBad = [](STAmount const& amount)
    {
        if (!amount.native())
            return true;

        if (amount.xrp().drops() <= 0)
            return true;

        if (amount.xrp().drops() >= SYSTEM_CURRENCY_START)
            return true;

        return false;
    };

    if(before && before->getType() == ltESCROW)
        bad_ |= isBad((*before)[sfAmount]);

    if(after && after->getType() == ltESCROW)
        bad_ |= isBad((*after)[sfAmount]);
}

bool
NoZeroEscrow::finalize(STTx const& tx, TER const, XRPAmount const, beast::Journal const& j)
{
    if (bad_)
    {
        JLOG(j.fatal()) << "Invariant failed: escrow specifies invalid amount";
        return false;
    }

    return true;
}


void
AccountRootsNotDeleted::visitEntry(
    uint256 const&,
    bool isDelete,
    std::shared_ptr <SLE const> const& before,
    std::shared_ptr <SLE const> const&)
{
    if (isDelete && before && before->getType() == ltACCOUNT_ROOT)
        accountDeleted_ = true;
}

bool
AccountRootsNotDeleted::finalize(STTx const&, TER const, XRPAmount const, beast::Journal const& j)
{
    if (! accountDeleted_)
        return true;

    JLOG(j.fatal()) << "Invariant failed: an account root was deleted";
    return false;
}


void
LedgerEntryTypesMatch::visitEntry(
    uint256 const&,
    bool,
    std::shared_ptr <SLE const> const& before,
    std::shared_ptr <SLE const> const& after)
{
    if (before && after && before->getType() != after->getType())
        typeMismatch_ = true;

    if (after)
    {
        switch (after->getType())
        {
        case ltACCOUNT_ROOT:
        case ltDIR_NODE:
        case ltRIPPLE_STATE:
        case ltTICKET:
        case ltSIGNER_LIST:
        case ltOFFER:
        case ltLEDGER_HASHES:
        case ltAMENDMENTS:
        case ltFEE_SETTINGS:
        case ltESCROW:
        case ltPAYCHAN:
        case ltCHECK:
        case ltDEPOSIT_PREAUTH:
            break;
        default:
            invalidTypeAdded_ = true;
            break;
        }
    }
}

bool
LedgerEntryTypesMatch::finalize(STTx const&, TER const, XRPAmount const, beast::Journal const& j)
{
    if ((! typeMismatch_) && (! invalidTypeAdded_))
        return true;

    if (typeMismatch_)
    {
        JLOG(j.fatal()) << "Invariant failed: ledger entry type mismatch";
    }

    if (invalidTypeAdded_)
    {
        JLOG(j.fatal()) << "Invariant failed: invalid ledger entry type added";
    }

    return false;
}


void
NoXRPTrustLines::visitEntry(
    uint256 const&,
    bool,
    std::shared_ptr <SLE const> const&,
    std::shared_ptr <SLE const> const& after)
{
    if (after && after->getType() == ltRIPPLE_STATE)
    {
        xrpTrustLine_ =
            after->getFieldAmount (sfLowLimit).issue() == xrpIssue() ||
            after->getFieldAmount (sfHighLimit).issue() == xrpIssue();
    }
}

bool
NoXRPTrustLines::finalize(STTx const&, TER const, XRPAmount const, beast::Journal const& j)
{
    if (! xrpTrustLine_)
        return true;

    JLOG(j.fatal()) << "Invariant failed: an XRP trust line was created";
    return false;
}

}  

























