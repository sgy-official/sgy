

#ifndef RIPPLE_RPC_TRANSACTIONSIGN_H_INCLUDED
#define RIPPLE_RPC_TRANSACTIONSIGN_H_INCLUDED

#include <ripple/app/misc/NetworkOPs.h>
#include <ripple/rpc/Role.h>
#include <ripple/ledger/ApplyView.h>

namespace ripple {

class Application;
class LoadFeeTrack;
class Transaction;
class TxQ;

namespace RPC {


Json::Value checkFee (
    Json::Value& request,
    Role const role,
    bool doAutoFill,
    Config const& config,
    LoadFeeTrack const& feeTrack,
    TxQ const& txQ,
    std::shared_ptr<OpenView const> const& ledger);

using ProcessTransactionFn =
    std::function<void (std::shared_ptr<Transaction>& transaction,
        bool bUnlimited, bool bLocal, NetworkOPs::FailHard failType)>;

inline ProcessTransactionFn getProcessTxnFn (NetworkOPs& netOPs)
{
    return [&netOPs](std::shared_ptr<Transaction>& transaction,
        bool bUnlimited, bool bLocal, NetworkOPs::FailHard failType)
    {
        netOPs.processTransaction(transaction, bUnlimited, bLocal, failType);
    };
}


Json::Value transactionSign (
    Json::Value params,  
    NetworkOPs::FailHard failType,
    Role role,
    std::chrono::seconds validatedLedgerAge,
    Application& app);


Json::Value transactionSubmit (
    Json::Value params,  
    NetworkOPs::FailHard failType,
    Role role,
    std::chrono::seconds validatedLedgerAge,
    Application& app,
    ProcessTransactionFn const& processTransaction);


Json::Value transactionSignFor (
    Json::Value params,  
    NetworkOPs::FailHard failType,
    Role role,
    std::chrono::seconds validatedLedgerAge,
    Application& app);


Json::Value transactionSubmitMultiSigned (
    Json::Value params,  
    NetworkOPs::FailHard failType,
    Role role,
    std::chrono::seconds validatedLedgerAge,
    Application& app,
    ProcessTransactionFn const& processTransaction);

} 
} 

#endif








