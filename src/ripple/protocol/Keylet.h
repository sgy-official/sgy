

#ifndef RIPPLE_PROTOCOL_KEYLET_H_INCLUDED
#define RIPPLE_PROTOCOL_KEYLET_H_INCLUDED

#include <ripple/protocol/LedgerFormats.h>
#include <ripple/basics/base_uint.h>

namespace ripple {

class STLedgerEntry;


struct Keylet
{
    LedgerEntryType type;
    uint256 key;

    Keylet (LedgerEntryType type_,
            uint256 const& key_)
        : type(type_)
        , key(key_)
    {
    }

    
    bool
    check (STLedgerEntry const&) const;
};

}

#endif








