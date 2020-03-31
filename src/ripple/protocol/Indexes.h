

#ifndef RIPPLE_PROTOCOL_INDEXES_H_INCLUDED
#define RIPPLE_PROTOCOL_INDEXES_H_INCLUDED

#include <ripple/protocol/Keylet.h>
#include <ripple/protocol/LedgerFormats.h>
#include <ripple/protocol/Protocol.h>
#include <ripple/protocol/PublicKey.h>
#include <ripple/protocol/Serializer.h>
#include <ripple/protocol/UintTypes.h>
#include <ripple/basics/base_uint.h>
#include <ripple/protocol/Book.h>
#include <cstdint>

namespace ripple {

uint256
getLedgerHashIndex ();

uint256
getLedgerHashIndex (std::uint32_t desiredLedgerIndex);

uint256
getLedgerAmendmentIndex ();

uint256
getLedgerFeeIndex ();

uint256
getAccountRootIndex (AccountID const& account);

uint256
getGeneratorIndex (AccountID const& uGeneratorID);

uint256
getBookBase (Book const& book);

uint256
getOfferIndex (AccountID const& account, std::uint32_t uSequence);

uint256
getOwnerDirIndex (AccountID const& account);

uint256
getDirNodeIndex (uint256 const& uDirRoot, const std::uint64_t uNodeIndex);

uint256
getQualityIndex (uint256 const& uBase, const std::uint64_t uNodeDir = 0);

uint256
getQualityNext (uint256 const& uBase);

std::uint64_t
getQuality (uint256 const& uBase);

uint256
getTicketIndex (AccountID const& account, std::uint32_t uSequence);

uint256
getRippleStateIndex (AccountID const& a, AccountID const& b, Currency const& currency);

uint256
getRippleStateIndex (AccountID const& a, Issue const& issue);

uint256
getSignerListIndex (AccountID const& account);

uint256
getCheckIndex (AccountID const& account, std::uint32_t uSequence);

uint256
getDepositPreauthIndex (AccountID const& owner, AccountID const& preauthorized);





namespace keylet {


struct account_t
{
    explicit account_t() = default;

    Keylet operator()(AccountID const& id) const;
};
static account_t const account {};


struct amendments_t
{
    explicit amendments_t() = default;

    Keylet operator()() const;
};
static amendments_t const amendments {};


Keylet child (uint256 const& key);


struct skip_t
{
    explicit skip_t() = default;

    Keylet operator()() const;

    Keylet operator()(LedgerIndex ledger) const;
};
static skip_t const skip {};


struct fees_t
{
    explicit fees_t() = default;

    Keylet operator()() const;
};
static fees_t const fees {};


struct book_t
{
    explicit book_t() = default;

    Keylet operator()(Book const& b) const;
};
static book_t const book {};


struct line_t
{
    explicit line_t() = default;

    Keylet operator()(AccountID const& id0,
        AccountID const& id1, Currency const& currency) const;

    Keylet operator()(AccountID const& id,
        Issue const& issue) const;

    Keylet operator()(uint256 const& key) const
    {
        return { ltRIPPLE_STATE, key };
    }
};
static line_t const line {};


struct offer_t
{
    explicit offer_t() = default;

    Keylet operator()(AccountID const& id,
        std::uint32_t seq) const;

    Keylet operator()(uint256 const& key) const
    {
        return { ltOFFER, key };
    }
};
static offer_t const offer {};


struct quality_t
{
    explicit quality_t() = default;

    Keylet operator()(Keylet const& k,
        std::uint64_t q) const;
};
static quality_t const quality {};


struct next_t
{
    explicit next_t() = default;

    Keylet operator()(Keylet const& k) const;
};
static next_t const next {};


struct ticket_t
{
    explicit ticket_t() = default;

    Keylet operator()(AccountID const& id,
        std::uint32_t seq) const;

    Keylet operator()(uint256 const& key) const
    {
        return { ltTICKET, key };
    }
};
static ticket_t const ticket {};


struct signers_t
{
    explicit signers_t() = default;

    Keylet operator()(AccountID const& id) const;

    Keylet operator()(uint256 const& key) const
    {
        return { ltSIGNER_LIST, key };
    }
};
static signers_t const signers {};


struct check_t
{
    explicit check_t() = default;

    Keylet operator()(AccountID const& id,
        std::uint32_t seq) const;

    Keylet operator()(uint256 const& key) const
    {
        return { ltCHECK, key };
    }
};
static check_t const check {};


struct depositPreauth_t
{
    explicit depositPreauth_t() = default;

    Keylet operator()(AccountID const& owner,
        AccountID const& preauthorized) const;

    Keylet operator()(uint256 const& key) const
    {
        return { ltDEPOSIT_PREAUTH, key };
    }
};
static depositPreauth_t const depositPreauth {};



Keylet unchecked(uint256 const& key);


Keylet ownerDir (AccountID const& id);



Keylet page (uint256 const& root, std::uint64_t index);
Keylet page (Keylet const& root, std::uint64_t index);


inline
Keylet page (uint256 const& key)
{
    return { ltDIR_NODE, key };
}


Keylet
escrow (AccountID const& source, std::uint32_t seq);


Keylet
payChan (AccountID const& source, AccountID const& dst, std::uint32_t seq);

} 

}

#endif








