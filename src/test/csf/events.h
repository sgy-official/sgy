
#ifndef RIPPLE_TEST_CSF_EVENTS_H_INCLUDED
#define RIPPLE_TEST_CSF_EVENTS_H_INCLUDED

#include <test/csf/Tx.h>
#include <test/csf/Validation.h>
#include <test/csf/ledgers.h>
#include <test/csf/Proposal.h>
#include <chrono>


namespace ripple {
namespace test {
namespace csf {





template <class V>
struct Share
{
    V val;
};


template <class V>
struct Relay
{
    PeerID to;

    V val;
};


template <class V>
struct Receive
{
    PeerID from;

    V val;
};


struct SubmitTx
{
    Tx tx;
};


struct StartRound
{
    Ledger::ID bestLedger;

    Ledger prevLedger;
};


struct CloseLedger
{
    Ledger prevLedger;

    TxSetType txs;
};

struct AcceptLedger
{
    Ledger ledger;

    Ledger prior;
};

struct WrongPrevLedger
{
    Ledger::ID wrong;
    Ledger::ID right;
};

struct FullyValidateLedger
{
    Ledger ledger;

    Ledger prior;
};


}  
}  
}  

#endif








