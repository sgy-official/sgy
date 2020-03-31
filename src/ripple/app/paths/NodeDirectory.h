

#ifndef RIPPLE_APP_PATHS_NODEDIRECTORY_H_INCLUDED
#define RIPPLE_APP_PATHS_NODEDIRECTORY_H_INCLUDED

#include <ripple/protocol/Indexes.h>
#include <ripple/protocol/STLedgerEntry.h>
#include <ripple/ledger/ApplyView.h>

namespace ripple {


class NodeDirectory
{
public:
    explicit NodeDirectory() = default;

    uint256 current;

    uint256 next;


    bool advanceNeeded;  
    bool restartNeeded;  

    SLE::pointer ledgerEntry;

    void restart (bool multiQuality)
    {
        if (multiQuality)
            current = beast::zero;             
        else
            restartNeeded  = true;   
    }

    bool initialize (Book const& book, ApplyView& view)
    {
        if (current != beast::zero)
            return false;

        current = getBookBase(book);
        next = getQualityNext(current);

        ledgerEntry = view.peek (keylet::page(current));

        advanceNeeded  = ! ledgerEntry;
        restartNeeded  = false;

        return bool(ledgerEntry);
    }

    enum Advance
    {
        NO_ADVANCE,
        NEW_QUALITY,
        END_ADVANCE
    };

    
    Advance
    advance (ApplyView& view)
    {
        if (!(advanceNeeded || restartNeeded))
            return NO_ADVANCE;

        if (advanceNeeded)
        {
            auto const opt =
                view.succ (current, next);
            current = opt ? *opt : uint256{};
        }
        advanceNeeded  = false;
        restartNeeded  = false;

        if (current == beast::zero)
            return END_ADVANCE;

        ledgerEntry = view.peek (keylet::page(current));
        return NEW_QUALITY;
    }
};

} 

#endif








