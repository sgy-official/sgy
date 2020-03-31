

#ifndef RIPPLE_APP_MISC_TRANSACTION_H_INCLUDED
#define RIPPLE_APP_MISC_TRANSACTION_H_INCLUDED

#include <ripple/protocol/Protocol.h>
#include <ripple/protocol/STTx.h>
#include <ripple/protocol/TER.h>
#include <ripple/beast/utility/Journal.h>
#include <boost/optional.hpp>

namespace ripple {


class Application;
class Database;
class Rules;

enum TransStatus
{
    NEW         = 0, 
    INVALID     = 1, 
    INCLUDED    = 2, 
    CONFLICTED  = 3, 
    COMMITTED   = 4, 
    HELD        = 5, 
    REMOVED     = 6, 
    OBSOLETE    = 7, 
    INCOMPLETE  = 8  
};

class Transaction
    : public std::enable_shared_from_this<Transaction>
    , public CountedObject <Transaction>
{
public:
    static char const* getCountedObjectName () { return "Transaction"; }

    using pointer = std::shared_ptr<Transaction>;
    using ref = const pointer&;

public:
    Transaction (
        std::shared_ptr<STTx const> const&, std::string&, Application&) noexcept;

    static
    Transaction::pointer
    transactionFromSQL (
        boost::optional<std::uint64_t> const& ledgerSeq,
        boost::optional<std::string> const& status,
        Blob const& rawTxn,
        Application& app);

    static
    Transaction::pointer
    transactionFromSQLValidated (
        boost::optional<std::uint64_t> const& ledgerSeq,
        boost::optional<std::string> const& status,
        Blob const& rawTxn,
        Application& app);

    static
    TransStatus
    sqlTransactionStatus(boost::optional<std::string> const& status);

    std::shared_ptr<STTx const> const& getSTransaction ()
    {
        return mTransaction;
    }

    uint256 const& getID () const
    {
        return mTransactionID;
    }

    LedgerIndex getLedger () const
    {
        return mInLedger;
    }

    TransStatus getStatus () const
    {
        return mStatus;
    }

    TER getResult ()
    {
        return mResult;
    }

    void setResult (TER terResult)
    {
        mResult = terResult;
    }

    void setStatus (TransStatus status, std::uint32_t ledgerSeq);

    void setStatus (TransStatus status)
    {
        mStatus = status;
    }

    void setLedger (LedgerIndex ledger)
    {
        mInLedger = ledger;
    }

    
    void setApplying()
    {
        mApplying = true;
    }

    
    bool getApplying()
    {
        return mApplying;
    }

    
    void clearApplying()
    {
        mApplying = false;
    }

    Json::Value getJson (JsonOptions options, bool binary = false) const;

    static Transaction::pointer load (uint256 const& id, Application& app);

private:
    uint256         mTransactionID;

    LedgerIndex     mInLedger = 0;
    TransStatus     mStatus = INVALID;
    TER             mResult = temUNCERTAIN;
    bool            mApplying = false;

    std::shared_ptr<STTx const>   mTransaction;
    Application&    mApp;
    beast::Journal  j_;
};

} 

#endif








