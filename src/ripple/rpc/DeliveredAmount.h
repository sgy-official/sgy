

#ifndef RIPPLE_RPC_DELIVEREDAMOUNT_H_INCLUDED
#define RIPPLE_RPC_DELIVEREDAMOUNT_H_INCLUDED

#include <memory>

namespace Json {
class Value;
}

namespace ripple {

class ReadView;
class Transaction;
class TxMeta;
class STTx;

namespace RPC {

struct Context;


void
insertDeliveredAmount(
    Json::Value& meta,
    ReadView const&,
    std::shared_ptr<STTx const> serializedTx,
    TxMeta const&);

void
insertDeliveredAmount(
    Json::Value& meta,
    Context&,
    std::shared_ptr<Transaction>,
    TxMeta const&);



} 
} 

#endif








