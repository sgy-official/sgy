

#include <ripple/protocol/LedgerFormats.h>
#include <ripple/protocol/ErrorCodes.h>
#include <ripple/protocol/jss.h>
#include <algorithm>
#include <array>
#include <utility>

namespace ripple {

LedgerFormats::LedgerFormats ()
{
    static const std::initializer_list<SOElement> commonFields
    {
        { sfLedgerIndex,             soeOPTIONAL },
        { sfLedgerEntryType,         soeREQUIRED },
        { sfFlags,                   soeREQUIRED },
    };

    add (jss::AccountRoot, ltACCOUNT_ROOT,
        {
            { sfAccount,             soeREQUIRED },
            { sfSequence,            soeREQUIRED },
            { sfBalance,             soeREQUIRED },
            { sfOwnerCount,          soeREQUIRED },
            { sfPreviousTxnID,       soeREQUIRED },
            { sfPreviousTxnLgrSeq,   soeREQUIRED },
            { sfAccountTxnID,        soeOPTIONAL },
            { sfRegularKey,          soeOPTIONAL },
            { sfEmailHash,           soeOPTIONAL },
            { sfWalletLocator,       soeOPTIONAL },
            { sfWalletSize,          soeOPTIONAL },
            { sfMessageKey,          soeOPTIONAL },
            { sfTransferRate,        soeOPTIONAL },
            { sfDomain,              soeOPTIONAL },
            { sfTickSize,            soeOPTIONAL },
        },
        commonFields);

    add (jss::DirectoryNode, ltDIR_NODE,
        {
            { sfOwner,               soeOPTIONAL },  
            { sfTakerPaysCurrency,   soeOPTIONAL },  
            { sfTakerPaysIssuer,     soeOPTIONAL },  
            { sfTakerGetsCurrency,   soeOPTIONAL },  
            { sfTakerGetsIssuer,     soeOPTIONAL },  
            { sfExchangeRate,        soeOPTIONAL },  
            { sfIndexes,             soeREQUIRED },
            { sfRootIndex,           soeREQUIRED },
            { sfIndexNext,           soeOPTIONAL },
            { sfIndexPrevious,       soeOPTIONAL },
        },
        commonFields);

    add (jss::Offer, ltOFFER,
        {
            { sfAccount,             soeREQUIRED },
            { sfSequence,            soeREQUIRED },
            { sfTakerPays,           soeREQUIRED },
            { sfTakerGets,           soeREQUIRED },
            { sfBookDirectory,       soeREQUIRED },
            { sfBookNode,            soeREQUIRED },
            { sfOwnerNode,           soeREQUIRED },
            { sfPreviousTxnID,       soeREQUIRED },
            { sfPreviousTxnLgrSeq,   soeREQUIRED },
            { sfExpiration,          soeOPTIONAL },
        },
        commonFields);

    add (jss::RippleState, ltRIPPLE_STATE,
        {
            { sfBalance,             soeREQUIRED },
            { sfLowLimit,            soeREQUIRED },
            { sfHighLimit,           soeREQUIRED },
            { sfPreviousTxnID,       soeREQUIRED },
            { sfPreviousTxnLgrSeq,   soeREQUIRED },
            { sfLowNode,             soeOPTIONAL },
            { sfLowQualityIn,        soeOPTIONAL },
            { sfLowQualityOut,       soeOPTIONAL },
            { sfHighNode,            soeOPTIONAL },
            { sfHighQualityIn,       soeOPTIONAL },
            { sfHighQualityOut,      soeOPTIONAL },
        },
        commonFields);

    add (jss::Escrow, ltESCROW,
        {
            { sfAccount,             soeREQUIRED },
            { sfDestination,         soeREQUIRED },
            { sfAmount,              soeREQUIRED },
            { sfCondition,           soeOPTIONAL },
            { sfCancelAfter,         soeOPTIONAL },
            { sfFinishAfter,         soeOPTIONAL },
            { sfSourceTag,           soeOPTIONAL },
            { sfDestinationTag,      soeOPTIONAL },
            { sfOwnerNode,           soeREQUIRED },
            { sfPreviousTxnID,       soeREQUIRED },
            { sfPreviousTxnLgrSeq,   soeREQUIRED },
            { sfDestinationNode,     soeOPTIONAL },
        },
        commonFields);

    add (jss::LedgerHashes, ltLEDGER_HASHES,
        {
            { sfFirstLedgerSequence, soeOPTIONAL }, 
            { sfLastLedgerSequence,  soeOPTIONAL },
            { sfHashes,              soeREQUIRED },
        },
        commonFields);

    add (jss::Amendments, ltAMENDMENTS,
        {
            { sfAmendments,          soeOPTIONAL }, 
            { sfMajorities,          soeOPTIONAL },
        },
        commonFields);

    add (jss::FeeSettings, ltFEE_SETTINGS,
        {
            { sfBaseFee,             soeREQUIRED },
            { sfReferenceFeeUnits,   soeREQUIRED },
            { sfReserveBase,         soeREQUIRED },
            { sfReserveIncrement,    soeREQUIRED },
        },
        commonFields);

    add (jss::Ticket, ltTICKET,
        {
            { sfAccount,             soeREQUIRED },
            { sfSequence,            soeREQUIRED },
            { sfOwnerNode,           soeREQUIRED },
            { sfTarget,              soeOPTIONAL },
            { sfExpiration,          soeOPTIONAL },
        },
        commonFields);

    add (jss::SignerList, ltSIGNER_LIST,
        {
            { sfOwnerNode,           soeREQUIRED },
            { sfSignerQuorum,        soeREQUIRED },
            { sfSignerEntries,       soeREQUIRED },
            { sfSignerListID,        soeREQUIRED },
            { sfPreviousTxnID,       soeREQUIRED },
            { sfPreviousTxnLgrSeq,   soeREQUIRED },
        },
        commonFields);

    add (jss::PayChannel, ltPAYCHAN,
        {
            { sfAccount,             soeREQUIRED },
            { sfDestination,         soeREQUIRED },
            { sfAmount,              soeREQUIRED },
            { sfBalance,             soeREQUIRED },
            { sfPublicKey,           soeREQUIRED },
            { sfSettleDelay,         soeREQUIRED },
            { sfExpiration,          soeOPTIONAL },
            { sfCancelAfter,         soeOPTIONAL },
            { sfSourceTag,           soeOPTIONAL },
            { sfDestinationTag,      soeOPTIONAL },
            { sfOwnerNode,           soeREQUIRED },
            { sfPreviousTxnID,       soeREQUIRED },
            { sfPreviousTxnLgrSeq,   soeREQUIRED },
        },
        commonFields);

    add (jss::Check, ltCHECK,
        {
            { sfAccount,             soeREQUIRED },
            { sfDestination,         soeREQUIRED },
            { sfSendMax,             soeREQUIRED },
            { sfSequence,            soeREQUIRED },
            { sfOwnerNode,           soeREQUIRED },
            { sfDestinationNode,     soeREQUIRED },
            { sfExpiration,          soeOPTIONAL },
            { sfInvoiceID,           soeOPTIONAL },
            { sfSourceTag,           soeOPTIONAL },
            { sfDestinationTag,      soeOPTIONAL },
            { sfPreviousTxnID,       soeREQUIRED },
            { sfPreviousTxnLgrSeq,   soeREQUIRED },
        },
        commonFields);

    add (jss::DepositPreauth, ltDEPOSIT_PREAUTH,
        {
            { sfAccount,             soeREQUIRED },
            { sfAuthorize,           soeREQUIRED },
            { sfOwnerNode,           soeREQUIRED },
            { sfPreviousTxnID,       soeREQUIRED },
            { sfPreviousTxnLgrSeq,   soeREQUIRED },
        },
        commonFields);
}

LedgerFormats const&
LedgerFormats::getInstance ()
{
    static LedgerFormats instance;
    return instance;
}

} 
























