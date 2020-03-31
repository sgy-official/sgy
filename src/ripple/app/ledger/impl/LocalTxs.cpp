

#include <ripple/app/ledger/Ledger.h>
#include <ripple/app/ledger/LocalTxs.h>
#include <ripple/app/main/Application.h>
#include <ripple/protocol/Indexes.h>



namespace ripple {

class LocalTx
{
public:

    static int const holdLedgers = 5;

    LocalTx (LedgerIndex index, std::shared_ptr<STTx const> const& txn)
        : m_txn (txn)
        , m_expire (index + holdLedgers)
        , m_id (txn->getTransactionID ())
        , m_account (txn->getAccountID(sfAccount))
        , m_seq (txn->getSequence())
    {
        if (txn->isFieldPresent (sfLastLedgerSequence))
            m_expire = std::min (m_expire, txn->getFieldU32 (sfLastLedgerSequence) + 1);
    }

    uint256 const& getID () const
    {
        return m_id;
    }

    std::uint32_t getSeq () const
    {
        return m_seq;
    }

    bool isExpired (LedgerIndex i) const
    {
        return i > m_expire;
    }

    std::shared_ptr<STTx const> const& getTX () const
    {
        return m_txn;
    }

    AccountID const& getAccount () const
    {
        return m_account;
    }

private:

    std::shared_ptr<STTx const> m_txn;
    LedgerIndex                    m_expire;
    uint256                        m_id;
    AccountID                      m_account;
    std::uint32_t                  m_seq;
};


class LocalTxsImp
    : public LocalTxs
{
public:
    LocalTxsImp() = default;

    void push_back (LedgerIndex index, std::shared_ptr<STTx const> const& txn) override
    {
        std::lock_guard <std::mutex> lock (m_lock);

        m_txns.emplace_back (index, txn);
    }

    CanonicalTXSet
    getTxSet () override
    {
        CanonicalTXSet tset (uint256 {});

        {
            std::lock_guard <std::mutex> lock (m_lock);

            for (auto const& it : m_txns)
                tset.insert (it.getTX());
        }

        return tset;
    }

    void sweep (ReadView const& view) override
    {
        std::lock_guard <std::mutex> lock (m_lock);

        m_txns.remove_if ([&view](auto const& txn)
        {
            if (txn.isExpired (view.info().seq))
                return true;
            if (view.txExists(txn.getID()))
                return true;

            std::shared_ptr<SLE const> sle = view.read(
                 keylet::account(txn.getAccount()));
            if (! sle)
                return false;
            return sle->getFieldU32 (sfSequence) > txn.getSeq ();
        });
    }

    std::size_t size () override
    {
        std::lock_guard <std::mutex> lock (m_lock);

        return m_txns.size ();
    }

private:
    std::mutex m_lock;
    std::list <LocalTx> m_txns;
};

std::unique_ptr<LocalTxs>
make_LocalTxs ()
{
    return std::make_unique<LocalTxsImp> ();
}

} 
























