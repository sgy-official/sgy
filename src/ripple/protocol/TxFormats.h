

#ifndef RIPPLE_PROTOCOL_TXFORMATS_H_INCLUDED
#define RIPPLE_PROTOCOL_TXFORMATS_H_INCLUDED

#include <ripple/protocol/KnownFormats.h>

namespace ripple {


enum TxType
{
    ttINVALID            =  -1,

    ttPAYMENT            =   0,
    ttESCROW_CREATE      =   1,
    ttESCROW_FINISH      =   2,
    ttACCOUNT_SET        =   3,
    ttESCROW_CANCEL      =   4,
    ttREGULAR_KEY_SET    =   5,
    ttNICKNAME_SET       =   6, 
    ttOFFER_CREATE       =   7,
    ttOFFER_CANCEL       =   8,
    no_longer_used       =   9,
    ttTICKET_CREATE      =  10,
    ttTICKET_CANCEL      =  11,
    ttSIGNER_LIST_SET    =  12,
    ttPAYCHAN_CREATE     =  13,
    ttPAYCHAN_FUND       =  14,
    ttPAYCHAN_CLAIM      =  15,
    ttCHECK_CREATE       =  16,
    ttCHECK_CASH         =  17,
    ttCHECK_CANCEL       =  18,
    ttDEPOSIT_PREAUTH    =  19,
    ttTRUST_SET          =  20,

    ttAMENDMENT          = 100,
    ttFEE                = 101,
};


class TxFormats : public KnownFormats <TxType>
{
private:
    
    TxFormats ();

public:
    static TxFormats const& getInstance ();
};

} 

#endif








