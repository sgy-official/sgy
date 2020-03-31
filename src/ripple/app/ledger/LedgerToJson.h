

#ifndef RIPPLE_APP_LEDGER_LEDGERTOJSON_H_INCLUDED
#define RIPPLE_APP_LEDGER_LEDGERTOJSON_H_INCLUDED

#include <ripple/app/ledger/Ledger.h>
#include <ripple/app/misc/TxQ.h>
#include <ripple/basics/StringUtilities.h>
#include <ripple/protocol/jss.h>
#include <ripple/protocol/STTx.h>
#include <ripple/json/Object.h>

namespace ripple {

struct LedgerFill
{
    LedgerFill (ReadView const& l, int o = 0, std::vector<TxQ::TxDetails> q = {},
                LedgerEntryType t = ltINVALID)
        : ledger (l)
        , options (o)
        , txQueue(std::move(q))
        , type (t)
    {
    }

    enum Options {
        dumpTxrp = 1,
        dumpState = 2,
        expand = 4,
        full = 8,
        binary = 16,
        ownerFunds = 32,
        dumpQueue = 64
    };

    ReadView const& ledger;
    int options;
    std::vector<TxQ::TxDetails> txQueue;
    LedgerEntryType type;
};



void addJson(Json::Value&, LedgerFill const&);


Json::Value getJson (LedgerFill const&);


template <class Object>
Blob serializeBlob(Object const& o)
{
    Serializer s;
    o.add(s);
    return s.peekData();
}


inline
std::string serializeHex(STObject const& o)
{
    return strHex(serializeBlob(o));
}
} 

#endif








