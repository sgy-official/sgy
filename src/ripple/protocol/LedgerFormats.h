

#ifndef RIPPLE_PROTOCOL_LEDGERFORMATS_H_INCLUDED
#define RIPPLE_PROTOCOL_LEDGERFORMATS_H_INCLUDED

#include <ripple/protocol/KnownFormats.h>

namespace ripple {


enum LedgerEntryType
{
    
    ltANY = -3,

    
    ltCHILD             = -2,

    ltINVALID           = -1,


    ltACCOUNT_ROOT      = 'a',

    
    ltDIR_NODE          = 'd',

    ltRIPPLE_STATE      = 'r',

    ltTICKET            = 'T',

    ltSIGNER_LIST       = 'S',

    ltOFFER             = 'o',

    ltLEDGER_HASHES     = 'h',

    ltAMENDMENTS        = 'f',

    ltFEE_SETTINGS      = 's',

    ltESCROW            = 'u',

    ltPAYCHAN           = 'x',

    ltCHECK             = 'C',

    ltDEPOSIT_PREAUTH   = 'p',

    ltNICKNAME          = 'n',

    ltNotUsed01         = 'c',
};


enum LedgerNameSpace
{
    spaceAccount        = 'a',
    spaceDirNode        = 'd',
    spaceGenerator      = 'g',
    spaceRipple         = 'r',
    spaceOffer          = 'o',  
    spaceOwnerDir       = 'O',  
    spaceBookDir        = 'B',  
    spaceContract       = 'c',
    spaceSkipList       = 's',
    spaceEscrow         = 'u',
    spaceAmendment      = 'f',
    spaceFee            = 'e',
    spaceTicket         = 'T',
    spaceSignerList     = 'S',
    spaceXRPUChannel    = 'x',
    spaceCheck          = 'C',
    spaceDepositPreauth = 'p',

    spaceNickname       = 'n',
};


enum LedgerSpecificFlags
{
    lsfPasswordSpent    = 0x00010000,   
    lsfRequireDestTag   = 0x00020000,   
    lsfRequireAuth      = 0x00040000,   
    lsfDisallowXRP      = 0x00080000,   
    lsfDisableMaster    = 0x00100000,   
    lsfNoFreeze         = 0x00200000,   
    lsfGlobalFreeze     = 0x00400000,   
    lsfDefaultRipple    = 0x00800000,   
    lsfDepositAuth      = 0x01000000,   

    lsfPassive          = 0x00010000,
    lsfSell             = 0x00020000,   

    lsfLowReserve       = 0x00010000,   
    lsfHighReserve      = 0x00020000,
    lsfLowAuth          = 0x00040000,
    lsfHighAuth         = 0x00080000,
    lsfLowNoRipple      = 0x00100000,
    lsfHighNoRipple     = 0x00200000,
    lsfLowFreeze        = 0x00400000,   
    lsfHighFreeze       = 0x00800000,   

    lsfOneOwnerCount    = 0x00010000,   
};



class LedgerFormats : public KnownFormats <LedgerEntryType>
{
private:
    
    LedgerFormats ();

public:
    static LedgerFormats const& getInstance ();
};

} 

#endif








