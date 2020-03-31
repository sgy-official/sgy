

#include <ripple/rpc/DeliveredAmount.h>

#include <ripple/app/ledger/LedgerMaster.h>
#include <ripple/app/ledger/OpenLedger.h>
#include <ripple/app/misc/Transaction.h>
#include <ripple/ledger/View.h>
#include <ripple/net/RPCErr.h>
#include <ripple/protocol/AccountID.h>
#include <ripple/protocol/Feature.h>
#include <ripple/rpc/Context.h>
#include <ripple/rpc/impl/RPCHelpers.h>
#include <boost/algorithm/string/case_conv.hpp>

namespace ripple {
namespace RPC {


template<class GetFix1623Enabled, class GetLedgerIndex, class GetCloseTime>
void
insertDeliveredAmount(
    Json::Value& meta,
    GetFix1623Enabled const& getFix1623Enabled,
    GetLedgerIndex const& getLedgerIndex,
    GetCloseTime const& getCloseTime,
    std::shared_ptr<STTx const> serializedTx,
    TxMeta const& transactionMeta)
{
    {
        TxType const tt{serializedTx->getTxnType()};
        if (tt != ttPAYMENT &&
            tt != ttCHECK_CASH)
            return;

        if (tt == ttCHECK_CASH &&
            !getFix1623Enabled())
            return;
    }

    if (transactionMeta.getResultTER() != tesSUCCESS)
        return;

    if (transactionMeta.hasDeliveredAmount())
    {
        meta[jss::delivered_amount] =
            transactionMeta.getDeliveredAmount()
                .getJson(JsonOptions::include_date);
        return;
    }

    if (serializedTx->isFieldPresent(sfAmount))
    {
        using namespace std::chrono_literals;

        if (getLedgerIndex() >= 4594095 ||
            getCloseTime() > NetClock::time_point{446000000s})
        {
            meta[jss::delivered_amount] =
                serializedTx->getFieldAmount(sfAmount)
                   .getJson(JsonOptions::include_date);
            return;
        }
    }

    meta[jss::delivered_amount] = Json::Value("unavailable");
}

void
insertDeliveredAmount(
    Json::Value& meta,
    ReadView const& ledger,
    std::shared_ptr<STTx const> serializedTx,
    TxMeta const& transactionMeta)
{
    if (!serializedTx)
        return;

    auto const info = ledger.info();
    auto const getFix1623Enabled = [&ledger] {
        return ledger.rules().enabled(fix1623);
    };
    auto const getLedgerIndex = [&info] {
        return info.seq;
    };
    auto const getCloseTime = [&info] {
        return info.closeTime;
    };

    insertDeliveredAmount(
        meta,
        getFix1623Enabled,
        getLedgerIndex,
        getCloseTime,
        std::move(serializedTx),
        transactionMeta);
}

void
insertDeliveredAmount(
    Json::Value& meta,
    RPC::Context& context,
    std::shared_ptr<Transaction> transaction,
    TxMeta const& transactionMeta)
{
    if (!transaction)
        return;

    auto const serializedTx = transaction->getSTransaction ();
    if (! serializedTx)
        return;


    auto const getFix1623Enabled = [&context]() -> bool {
        auto const view = context.app.openLedger().current();
        if (!view)
            return false;
        return view->rules().enabled(fix1623);
    };
    auto const getLedgerIndex = [&transaction]() -> LedgerIndex {
        return transaction->getLedger();
    };
    auto const getCloseTime =
        [&context, &transaction]() -> boost::optional<NetClock::time_point> {
        return context.ledgerMaster.getCloseTimeBySeq(transaction->getLedger());
    };

    insertDeliveredAmount(
        meta,
        getFix1623Enabled,
        getLedgerIndex,
        getCloseTime,
        std::move(serializedTx),
        transactionMeta);
}
} 
} 
























