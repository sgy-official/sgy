

#ifndef RIPPLE_PROTOCOL_TXFLAGS_H_INCLUDED
#define RIPPLE_PROTOCOL_TXFLAGS_H_INCLUDED

#include <cstdint>

namespace ripple {



class TxFlag
{
public:
    explicit TxFlag() = default;

    static std::uint32_t const requireDestTag = 0x00010000;
};

const std::uint32_t tfFullyCanonicalSig    = 0x80000000;
const std::uint32_t tfUniversal            = tfFullyCanonicalSig;
const std::uint32_t tfUniversalMask        = ~ tfUniversal;

const std::uint32_t tfOptionalDestTag      = 0x00020000;
const std::uint32_t tfRequireAuth          = 0x00040000;
const std::uint32_t tfOptionalAuth         = 0x00080000;
const std::uint32_t tfDisallowXRP          = 0x00100000;
const std::uint32_t tfAllowXRP             = 0x00200000;
const std::uint32_t tfAccountSetMask       = ~ (tfUniversal | TxFlag::requireDestTag | tfOptionalDestTag
                                             | tfRequireAuth | tfOptionalAuth
                                             | tfDisallowXRP | tfAllowXRP);

const std::uint32_t asfRequireDest         = 1;
const std::uint32_t asfRequireAuth         = 2;
const std::uint32_t asfDisallowXRP         = 3;
const std::uint32_t asfDisableMaster       = 4;
const std::uint32_t asfAccountTxnID        = 5;
const std::uint32_t asfNoFreeze            = 6;
const std::uint32_t asfGlobalFreeze        = 7;
const std::uint32_t asfDefaultRipple       = 8;
const std::uint32_t asfDepositAuth         = 9;

const std::uint32_t tfPassive              = 0x00010000;
const std::uint32_t tfImmediateOrCancel    = 0x00020000;
const std::uint32_t tfFillOrKill           = 0x00040000;
const std::uint32_t tfSell                 = 0x00080000;
const std::uint32_t tfOfferCreateMask      = ~ (tfUniversal | tfPassive | tfImmediateOrCancel | tfFillOrKill | tfSell);

const std::uint32_t tfNoRippleDirect       = 0x00010000;
const std::uint32_t tfPartialPayment       = 0x00020000;
const std::uint32_t tfLimitQuality         = 0x00040000;
const std::uint32_t tfPaymentMask          = ~ (tfUniversal | tfPartialPayment | tfLimitQuality | tfNoRippleDirect);

const std::uint32_t tfSetfAuth             = 0x00010000;
const std::uint32_t tfSetNoRipple          = 0x00020000;
const std::uint32_t tfClearNoRipple        = 0x00040000;
const std::uint32_t tfSetFreeze            = 0x00100000;
const std::uint32_t tfClearFreeze          = 0x00200000;
const std::uint32_t tfTrustSetMask         = ~ (tfUniversal | tfSetfAuth | tfSetNoRipple | tfClearNoRipple
                                             | tfSetFreeze | tfClearFreeze);

const std::uint32_t tfGotMajority          = 0x00010000;
const std::uint32_t tfLostMajority         = 0x00020000;

const std::uint32_t tfRenew                = 0x00010000;
const std::uint32_t tfClose                = 0x00020000;
const std::uint32_t tfPayChanClaimMask     = ~ (tfUniversal | tfRenew | tfClose);

} 

#endif








