

#include <ripple/protocol/TxFormats.h>
#include <ripple/protocol/jss.h>

namespace ripple {

TxFormats::TxFormats ()
{
    static const std::initializer_list<SOElement> commonFields
    {
        { sfTransactionType,      soeREQUIRED },
        { sfFlags,                soeOPTIONAL },
        { sfSourceTag,            soeOPTIONAL },
        { sfAccount,              soeREQUIRED },
        { sfSequence,             soeREQUIRED },
        { sfPreviousTxnID,        soeOPTIONAL }, 
        { sfLastLedgerSequence,   soeOPTIONAL },
        { sfAccountTxnID,         soeOPTIONAL },
        { sfFee,                  soeREQUIRED },
        { sfOperationLimit,       soeOPTIONAL },
        { sfMemos,                soeOPTIONAL },
        { sfSigningPubKey,        soeREQUIRED },
        { sfTxnSignature,         soeOPTIONAL },
        { sfSigners,              soeOPTIONAL }, 
    };

    add (jss::AccountSet, ttACCOUNT_SET,
        {
            { sfEmailHash,           soeOPTIONAL },
            { sfWalletLocator,       soeOPTIONAL },
            { sfWalletSize,          soeOPTIONAL },
            { sfMessageKey,          soeOPTIONAL },
            { sfDomain,              soeOPTIONAL },
            { sfTransferRate,        soeOPTIONAL },
            { sfSetFlag,             soeOPTIONAL },
            { sfClearFlag,           soeOPTIONAL },
            { sfTickSize,            soeOPTIONAL },
        },
        commonFields);

    add (jss::TrustSet, ttTRUST_SET,
        {
            { sfLimitAmount,         soeOPTIONAL },
            { sfQualityIn,           soeOPTIONAL },
            { sfQualityOut,          soeOPTIONAL },
        },
        commonFields);

    add (jss::OfferCreate, ttOFFER_CREATE,
        {
            { sfTakerPays,           soeREQUIRED },
            { sfTakerGets,           soeREQUIRED },
            { sfExpiration,          soeOPTIONAL },
            { sfOfferSequence,       soeOPTIONAL },
        },
        commonFields);

    add (jss::OfferCancel, ttOFFER_CANCEL,
        {
            { sfOfferSequence,       soeREQUIRED },
        },
        commonFields);

    add (jss::SetRegularKey, ttREGULAR_KEY_SET,
        {
            { sfRegularKey,          soeOPTIONAL },
        },
        commonFields);

    add (jss::Payment, ttPAYMENT,
        {
            { sfDestination,         soeREQUIRED },
            { sfAmount,              soeREQUIRED },
            { sfSendMax,             soeOPTIONAL },
            { sfPaths,               soeDEFAULT  },
            { sfInvoiceID,           soeOPTIONAL },
            { sfDestinationTag,      soeOPTIONAL },
            { sfDeliverMin,          soeOPTIONAL },
        },
        commonFields);

    add (jss::EscrowCreate, ttESCROW_CREATE,
        {
            { sfDestination,         soeREQUIRED },
            { sfAmount,              soeREQUIRED },
            { sfCondition,           soeOPTIONAL },
            { sfCancelAfter,         soeOPTIONAL },
            { sfFinishAfter,         soeOPTIONAL },
            { sfDestinationTag,      soeOPTIONAL },
        },
        commonFields);

    add (jss::EscrowFinish, ttESCROW_FINISH,
        {
            { sfOwner,               soeREQUIRED },
            { sfOfferSequence,       soeREQUIRED },
            { sfFulfillment,         soeOPTIONAL },
            { sfCondition,           soeOPTIONAL },
        },
        commonFields);

    add (jss::EscrowCancel, ttESCROW_CANCEL,
        {
            { sfOwner,               soeREQUIRED },
            { sfOfferSequence,       soeREQUIRED },
        },
        commonFields);

    add (jss::EnableAmendment, ttAMENDMENT,
        {
            { sfLedgerSequence,      soeREQUIRED },
            { sfAmendment,           soeREQUIRED },
        },
        commonFields);

    add (jss::SetFee, ttFEE,
        {
            { sfLedgerSequence,      soeOPTIONAL },
            { sfBaseFee,             soeREQUIRED },
            { sfReferenceFeeUnits,   soeREQUIRED },
            { sfReserveBase,         soeREQUIRED },
            { sfReserveIncrement,    soeREQUIRED },
        },
        commonFields);

    add (jss::TicketCreate, ttTICKET_CREATE,
        {
            { sfTarget,              soeOPTIONAL },
            { sfExpiration,          soeOPTIONAL },
        },
        commonFields);

    add (jss::TicketCancel, ttTICKET_CANCEL,
        {
            { sfTicketID,            soeREQUIRED },
        },
        commonFields);

    add (jss::SignerListSet, ttSIGNER_LIST_SET,
        {
            { sfSignerQuorum,        soeREQUIRED },
            { sfSignerEntries,       soeOPTIONAL },
        },
        commonFields);

    add (jss::PaymentChannelCreate, ttPAYCHAN_CREATE,
        {
            { sfDestination,         soeREQUIRED },
            { sfAmount,              soeREQUIRED },
            { sfSettleDelay,         soeREQUIRED },
            { sfPublicKey,           soeREQUIRED },
            { sfCancelAfter,         soeOPTIONAL },
            { sfDestinationTag,      soeOPTIONAL },
        },
        commonFields);

    add (jss::PaymentChannelFund, ttPAYCHAN_FUND,
        {
            { sfPayChannel,          soeREQUIRED },
            { sfAmount,              soeREQUIRED },
            { sfExpiration,          soeOPTIONAL },
        },
        commonFields);

    add (jss::PaymentChannelClaim, ttPAYCHAN_CLAIM,
        {
            { sfPayChannel,          soeREQUIRED },
            { sfAmount,              soeOPTIONAL },
            { sfBalance,             soeOPTIONAL },
            { sfSignature,           soeOPTIONAL },
            { sfPublicKey,           soeOPTIONAL },
        },
        commonFields);

    add (jss::CheckCreate, ttCHECK_CREATE,
        {
            { sfDestination,         soeREQUIRED },
            { sfSendMax,             soeREQUIRED },
            { sfExpiration,          soeOPTIONAL },
            { sfDestinationTag,      soeOPTIONAL },
            { sfInvoiceID,           soeOPTIONAL },
        },
        commonFields);

    add (jss::CheckCash, ttCHECK_CASH,
        {
            { sfCheckID,             soeREQUIRED },
            { sfAmount,              soeOPTIONAL },
            { sfDeliverMin,          soeOPTIONAL },
        },
        commonFields);

    add (jss::CheckCancel, ttCHECK_CANCEL,
        {
            { sfCheckID,             soeREQUIRED },
        },
        commonFields);

    add (jss::DepositPreauth, ttDEPOSIT_PREAUTH,
        {
            { sfAuthorize,           soeOPTIONAL },
            { sfUnauthorize,         soeOPTIONAL },
        },
        commonFields);
}

TxFormats const&
TxFormats::getInstance ()
{
    static TxFormats const instance;
    return instance;
}

} 
























