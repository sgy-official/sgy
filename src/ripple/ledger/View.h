

#ifndef RIPPLE_LEDGER_VIEW_H_INCLUDED
#define RIPPLE_LEDGER_VIEW_H_INCLUDED

#include <ripple/ledger/ApplyView.h>
#include <ripple/ledger/OpenView.h>
#include <ripple/ledger/RawView.h>
#include <ripple/ledger/ReadView.h>
#include <ripple/protocol/Protocol.h>
#include <ripple/protocol/Rate.h>
#include <ripple/protocol/Serializer.h>
#include <ripple/protocol/STLedgerEntry.h>
#include <ripple/protocol/STObject.h>
#include <ripple/protocol/STTx.h>
#include <ripple/protocol/TER.h>
#include <ripple/core/Config.h>
#include <ripple/beast/utility/Journal.h>
#include <boost/optional.hpp>
#include <functional>
#include <map>
#include <memory>
#include <utility>

#include <vector>

namespace ripple {



enum FreezeHandling
{
    fhIGNORE_FREEZE,
    fhZERO_IF_FROZEN
};

bool
isGlobalFrozen (ReadView const& view,
    AccountID const& issuer);

bool
isFrozen (ReadView const& view, AccountID const& account,
    Currency const& currency, AccountID const& issuer);

STAmount
accountHolds (ReadView const& view,
    AccountID const& account, Currency const& currency,
        AccountID const& issuer, FreezeHandling zeroIfFrozen,
              beast::Journal j);

STAmount
accountFunds (ReadView const& view, AccountID const& id,
    STAmount const& saDefault, FreezeHandling freezeHandling,
        beast::Journal j);

XRPAmount
xrpLiquid (ReadView const& view, AccountID const& id,
    std::int32_t ownerCountAdj, beast::Journal j);


void
forEachItem (ReadView const& view, AccountID const& id,
    std::function<void (std::shared_ptr<SLE const> const&)> f);


bool
forEachItemAfter (ReadView const& view, AccountID const& id,
    uint256 const& after, std::uint64_t const hint,
        unsigned int limit, std::function<
            bool (std::shared_ptr<SLE const> const&)> f);

Rate
transferRate (ReadView const& view,
    AccountID const& issuer);


bool
dirIsEmpty (ReadView const& view,
    Keylet const& k);

bool
cdirFirst (ReadView const& view,
    uint256 const& uRootIndex,  
    std::shared_ptr<SLE const>& sleNode,      
    unsigned int& uDirEntry,    
    uint256& uEntryIndex,       
    beast::Journal j);

bool
cdirNext (ReadView const& view,
    uint256 const& uRootIndex,  
    std::shared_ptr<SLE const>& sleNode,      
    unsigned int& uDirEntry,    
    uint256& uEntryIndex,       
    beast::Journal j);

std::set <uint256>
getEnabledAmendments (ReadView const& view);

using majorityAmendments_t = std::map <uint256, NetClock::time_point>;
majorityAmendments_t
getMajorityAmendments (ReadView const& view);


boost::optional<uint256>
hashOfSeq (ReadView const& ledger, LedgerIndex seq,
    beast::Journal journal);


inline
LedgerIndex
getCandidateLedger (LedgerIndex requested)
{
    return (requested + 255) & (~255);
}


bool areCompatible (ReadView const& validLedger, ReadView const& testLedger,
    beast::Journal::Stream& s, const char* reason);

bool areCompatible (uint256 const& validHash, LedgerIndex validIndex,
    ReadView const& testLedger, beast::Journal::Stream& s, const char* reason);



void
adjustOwnerCount (ApplyView& view,
    std::shared_ptr<SLE> const& sle,
        std::int32_t amount, beast::Journal j);

bool
dirFirst (ApplyView& view,
    uint256 const& uRootIndex,  
    std::shared_ptr<SLE>& sleNode,      
    unsigned int& uDirEntry,    
    uint256& uEntryIndex,       
    beast::Journal j);

bool
dirNext (ApplyView& view,
    uint256 const& uRootIndex,  
    std::shared_ptr<SLE>& sleNode,      
    unsigned int& uDirEntry,    
    uint256& uEntryIndex,       
    beast::Journal j);

std::function<void (SLE::ref)>
describeOwnerDir(AccountID const& account);

boost::optional<std::uint64_t>
dirAdd (ApplyView& view,
    Keylet const&                       uRootIndex,
    uint256 const&                      uLedgerIndex,
    bool                                strictOrder,
    std::function<void (SLE::ref)>      fDescriber,
    beast::Journal j);


TER
trustCreate (ApplyView& view,
    const bool      bSrcHigh,
    AccountID const&  uSrcAccountID,
    AccountID const&  uDstAccountID,
    uint256 const&  uIndex,             
    SLE::ref        sleAccount,         
    const bool      bAuth,              
    const bool      bNoRipple,          
    const bool      bFreeze,            
    STAmount const& saBalance,          
    STAmount const& saLimit,            
    std::uint32_t uSrcQualityIn,
    std::uint32_t uSrcQualityOut,
    beast::Journal j);

TER
trustDelete (ApplyView& view,
    std::shared_ptr<SLE> const& sleRippleState,
        AccountID const& uLowAccountID,
            AccountID const& uHighAccountID,
                beast::Journal j);


TER
offerDelete (ApplyView& view,
    std::shared_ptr<SLE> const& sle,
        beast::Journal j);



TER
rippleCredit (ApplyView& view,
    AccountID const& uSenderID, AccountID const& uReceiverID,
    const STAmount & saAmount, bool bCheckIssuer,
    beast::Journal j);

TER
accountSend (ApplyView& view,
    AccountID const& from,
        AccountID const& to,
            const STAmount & saAmount,
                 beast::Journal j);

TER
issueIOU (ApplyView& view,
    AccountID const& account,
        STAmount const& amount,
            Issue const& issue,
                beast::Journal j);

TER
redeemIOU (ApplyView& view,
    AccountID const& account,
        STAmount const& amount,
            Issue const& issue,
                beast::Journal j);

TER
transferXRP (ApplyView& view,
    AccountID const& from,
        AccountID const& to,
            STAmount const& amount,
                beast::Journal j);

NetClock::time_point const& fix1141Time ();
bool fix1141 (NetClock::time_point const closeTime);

NetClock::time_point const& fix1274Time ();
bool fix1274 (NetClock::time_point const closeTime);

NetClock::time_point const& fix1298Time ();
bool fix1298 (NetClock::time_point const closeTime);

NetClock::time_point const& fix1443Time ();
bool fix1443 (NetClock::time_point const closeTime);

NetClock::time_point const& fix1449Time ();
bool fix1449 (NetClock::time_point const closeTime);

} 

#endif








