

#include <ripple/app/ledger/LedgerToJson.h>
#include <ripple/app/ledger/LedgerMaster.h>
#include <ripple/app/main/Application.h>
#include <ripple/app/misc/Transaction.h>
#include <ripple/app/misc/impl/AccountTxPaging.h>
#include <ripple/protocol/Serializer.h>
#include <ripple/protocol/UintTypes.h>
#include <boost/format.hpp>
#include <memory>

namespace ripple {

void
convertBlobsToTxResult (
    NetworkOPs::AccountTxs& to,
    std::uint32_t ledger_index,
    std::string const& status,
    Blob const& rawTxn,
    Blob const& rawMeta,
    Application& app)
{
    SerialIter it (makeSlice(rawTxn));
    auto txn = std::make_shared<STTx const> (it);
    std::string reason;

    auto tr = std::make_shared<Transaction> (txn, reason, app);

    tr->setStatus (Transaction::sqlTransactionStatus(status));
    tr->setLedger (ledger_index);

    auto metaset = std::make_shared<TxMeta> (
        tr->getID (), tr->getLedger (), rawMeta);

    to.emplace_back(std::move(tr), metaset);
};

void
saveLedgerAsync (Application& app, std::uint32_t seq)
{
    if (auto l = app.getLedgerMaster().getLedgerBySeq(seq))
        pendSaveValidated(app, l, false, false);
}

void
accountTxPage (
    DatabaseCon& connection,
    AccountIDCache const& idCache,
    std::function<void (std::uint32_t)> const& onUnsavedLedger,
    std::function<void (std::uint32_t,
                        std::string const&,
                        Blob const&,
                        Blob const&)> const& onTransaction,
    AccountID const& account,
    std::int32_t minLedger,
    std::int32_t maxLedger,
    bool forward,
    Json::Value& token,
    int limit,
    bool bAdmin,
    std::uint32_t page_length)
{
    bool lookingForMarker = token.isObject();

    std::uint32_t numberOfResults;

    if (limit <= 0 || (limit > page_length && !bAdmin))
        numberOfResults = page_length;
    else
        numberOfResults = limit;

    std::uint32_t queryLimit = numberOfResults + 1;
    std::uint32_t findLedger = 0, findSeq = 0;

    if (lookingForMarker)
    {
        try
        {
            if (!token.isMember(jss::ledger) || !token.isMember(jss::seq))
                return;
            findLedger = token[jss::ledger].asInt();
            findSeq = token[jss::seq].asInt();
        }
        catch (std::exception const&)
        {
            return;
        }
    }

    token = Json::nullValue;

    static std::string const prefix (
        R"(SELECT AccountTransactions.LedgerSeq,AccountTransactions.TxnSeq,
          Status,RawTxn,TxnMeta
          FROM AccountTransactions INNER JOIN Transactions
          ON Transactions.TransID = AccountTransactions.TransID
          AND AccountTransactions.Account = '%s' WHERE
          )");

    std::string sql;


    if (forward && (findLedger == 0))
    {
        sql = boost::str (boost::format(
            prefix +
            (R"(AccountTransactions.LedgerSeq BETWEEN '%u' AND '%u'
             ORDER BY AccountTransactions.LedgerSeq ASC,
             AccountTransactions.TxnSeq ASC
             LIMIT %u;)"))
            % idCache.toBase58(account)
            % minLedger
            % maxLedger
            % queryLimit);
    }
    else if (forward && (findLedger != 0))
    {
        auto b58acct = idCache.toBase58(account);
        sql = boost::str (boost::format(
            (R"(SELECT AccountTransactions.LedgerSeq,AccountTransactions.TxnSeq,
            Status,RawTxn,TxnMeta
            FROM AccountTransactions, Transactions WHERE
            (AccountTransactions.TransID = Transactions.TransID AND
            AccountTransactions.Account = '%s' AND
            AccountTransactions.LedgerSeq BETWEEN '%u' AND '%u')
            OR
            (AccountTransactions.TransID = Transactions.TransID AND
            AccountTransactions.Account = '%s' AND
            AccountTransactions.LedgerSeq = '%u' AND
            AccountTransactions.TxnSeq >= '%u')
            ORDER BY AccountTransactions.LedgerSeq ASC,
            AccountTransactions.TxnSeq ASC
            LIMIT %u;
            )"))
        % b58acct
        % (findLedger + 1)
        % maxLedger
        % b58acct
        % findLedger
        % findSeq
        % queryLimit);
    }
    else if (!forward && (findLedger == 0))
    {
        sql = boost::str (boost::format(
            prefix +
            (R"(AccountTransactions.LedgerSeq BETWEEN '%u' AND '%u'
             ORDER BY AccountTransactions.LedgerSeq DESC,
             AccountTransactions.TxnSeq DESC
             LIMIT %u;)"))
            % idCache.toBase58(account)
            % minLedger
            % maxLedger
            % queryLimit);
    }
    else if (!forward && (findLedger != 0))
    {
        auto b58acct = idCache.toBase58(account);
        sql = boost::str (boost::format(
            (R"(SELECT AccountTransactions.LedgerSeq,AccountTransactions.TxnSeq,
            Status,RawTxn,TxnMeta
            FROM AccountTransactions, Transactions WHERE
            (AccountTransactions.TransID = Transactions.TransID AND
            AccountTransactions.Account = '%s' AND
            AccountTransactions.LedgerSeq BETWEEN '%u' AND '%u')
            OR
            (AccountTransactions.TransID = Transactions.TransID AND
            AccountTransactions.Account = '%s' AND
            AccountTransactions.LedgerSeq = '%u' AND
            AccountTransactions.TxnSeq <= '%u')
            ORDER BY AccountTransactions.LedgerSeq DESC,
            AccountTransactions.TxnSeq DESC
            LIMIT %u;
            )"))
            % b58acct
            % minLedger
            % (findLedger - 1)
            % b58acct
            % findLedger
            % findSeq
            % queryLimit);
    }
    else
    {
        assert (false);
        return;
    }

    {
        auto db (connection.checkoutDb());

        Blob rawData;
        Blob rawMeta;

        boost::optional<std::uint64_t> ledgerSeq;
        boost::optional<std::uint32_t> txnSeq;
        boost::optional<std::string> status;
        soci::blob txnData (*db);
        soci::blob txnMeta (*db);
        soci::indicator dataPresent, metaPresent;

        soci::statement st = (db->prepare << sql,
            soci::into (ledgerSeq),
            soci::into (txnSeq),
            soci::into (status),
            soci::into (txnData, dataPresent),
            soci::into (txnMeta, metaPresent));

        st.execute ();

        while (st.fetch ())
        {
            if (lookingForMarker)
            {
                if (findLedger == ledgerSeq.value_or (0) &&
                    findSeq == txnSeq.value_or (0))
                {
                    lookingForMarker = false;
                }
            }
            else if (numberOfResults == 0)
            {
                token = Json::objectValue;
                token[jss::ledger] = rangeCheckedCast<std::uint32_t>(ledgerSeq.value_or (0));
                token[jss::seq] = txnSeq.value_or (0);
                break;
            }

            if (!lookingForMarker)
            {
                if (dataPresent == soci::i_ok)
                    convert (txnData, rawData);
                else
                    rawData.clear ();

                if (metaPresent == soci::i_ok)
                    convert (txnMeta, rawMeta);
                else
                    rawMeta.clear ();

                if (rawMeta.size() == 0)
                    onUnsavedLedger(ledgerSeq.value_or (0));

                onTransaction(rangeCheckedCast<std::uint32_t>(ledgerSeq.value_or (0)),
                    *status, rawData, rawMeta);
                --numberOfResults;
            }
        }
    }

    return;
}

}
























