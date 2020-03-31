

#ifndef RIPPLE_APP_LEDGER_LEDGERREPLAY_H_INCLUDED
#define RIPPLE_APP_LEDGER_LEDGERREPLAY_H_INCLUDED

#include <cstdint>
#include <map>
#include <memory>

namespace ripple {

class Ledger;
class STTx;

class LedgerReplay
{
    std::shared_ptr<Ledger const> parent_;
    std::shared_ptr<Ledger const> replay_;
    std::map<std::uint32_t, std::shared_ptr<STTx const>> orderedTxns_;

public:
    LedgerReplay(
        std::shared_ptr<Ledger const> parent,
        std::shared_ptr<Ledger const> replay);

    
    std::shared_ptr<Ledger const> const&
    parent() const
    {
        return parent_;
    }

    
    std::shared_ptr<Ledger const> const&
    replay() const
    {
        return replay_;
    }

    
    std::map<std::uint32_t, std::shared_ptr<STTx const>> const&
    orderedTxns() const
    {
        return orderedTxns_;
    }
};

}  

#endif








