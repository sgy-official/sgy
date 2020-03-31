

#ifndef RIPPLE_APP_LEDGER_LEDGERHOLDER_H_INCLUDED
#define RIPPLE_APP_LEDGER_LEDGERHOLDER_H_INCLUDED

#include <ripple/basics/contract.h>
#include <mutex>

namespace ripple {




class LedgerHolder
{
public:
    void set (std::shared_ptr<Ledger const> ledger)
    {
        if(! ledger)
            LogicError("LedgerHolder::set with nullptr");
        if(! ledger->isImmutable())
            LogicError("LedgerHolder::set with mutable Ledger");
        std::lock_guard <std::mutex> sl (m_lock);
        m_heldLedger = std::move(ledger);
    }

    std::shared_ptr<Ledger const> get ()
    {
        std::lock_guard <std::mutex> sl (m_lock);
        return m_heldLedger;
    }

    bool empty ()
    {
        std::lock_guard <std::mutex> sl (m_lock);
        return m_heldLedger == nullptr;
    }

private:
    std::mutex m_lock;
    std::shared_ptr<Ledger const> m_heldLedger;
};

} 

#endif








