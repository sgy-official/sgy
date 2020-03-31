
#ifndef RIPPLE_TEST_CSF_PROPOSAL_H_INCLUDED
#define RIPPLE_TEST_CSF_PROPOSAL_H_INCLUDED

#include <ripple/consensus/ConsensusProposal.h>
#include <test/csf/ledgers.h>
#include <test/csf/Tx.h>
#include <test/csf/Validation.h>

namespace ripple {
namespace test {
namespace csf {

using Proposal = ConsensusProposal<PeerID, Ledger::ID, TxSet::ID>;

}  
}  
}  

#endif







