

#ifndef RIPPLE_APP_TX_INVARIANTCHECK_H_INCLUDED
#define RIPPLE_APP_TX_INVARIANTCHECK_H_INCLUDED

#include <ripple/basics/base_uint.h>
#include <ripple/protocol/STLedgerEntry.h>
#include <ripple/protocol/STTx.h>
#include <ripple/protocol/TER.h>
#include <ripple/beast/utility/Journal.h>
#include <map>
#include <tuple>
#include <utility>
#include <cstdint>

namespace ripple {

#if GENERATING_DOCS

class InvariantChecker_PROTOTYPE
{
public:
    explicit InvariantChecker_PROTOTYPE() = default;

    
    void
    visitEntry(
        uint256 const& index,
        bool isDelete,
        std::shared_ptr<SLE const> const& before,
        std::shared_ptr<SLE const> const& after);

    
    bool
    finalize(
        STTx const& tx,
        TER const tec,
        XRPAmount const fee,
        beast::Journal const& j);
};
#endif


class TransactionFeeCheck
{
public:
    void
    visitEntry(
        uint256 const&,
        bool,
        std::shared_ptr<SLE const> const&,
        std::shared_ptr<SLE const> const&);

    bool
    finalize(STTx const&, TER const, XRPAmount const, beast::Journal const&);
};


class XRPNotCreated
{
    std::int64_t drops_ = 0;

public:
    void
    visitEntry(
        uint256 const&,
        bool,
        std::shared_ptr<SLE const> const&,
        std::shared_ptr<SLE const> const&);

    bool
    finalize(STTx const&, TER const, XRPAmount const, beast::Journal const&);
};


class AccountRootsNotDeleted
{
    bool accountDeleted_ = false;

public:
    void
    visitEntry(
        uint256 const&,
        bool,
        std::shared_ptr<SLE const> const&,
        std::shared_ptr<SLE const> const&);

    bool
    finalize(STTx const&, TER const, XRPAmount const, beast::Journal const&);
};


class XRPBalanceChecks
{
    bool bad_ = false;

public:
    void
    visitEntry(
        uint256 const&,
        bool,
        std::shared_ptr<SLE const> const&,
        std::shared_ptr<SLE const> const&);

    bool
    finalize(STTx const&, TER const, XRPAmount const, beast::Journal const&);
};


class LedgerEntryTypesMatch
{
    bool typeMismatch_ = false;
    bool invalidTypeAdded_ = false;

public:
    void
    visitEntry(
        uint256 const&,
        bool,
        std::shared_ptr<SLE const> const&,
        std::shared_ptr<SLE const> const&);

    bool
    finalize(STTx const&, TER const, XRPAmount const, beast::Journal const&);
};


class NoXRPTrustLines
{
    bool xrpTrustLine_ = false;

public:
    void
    visitEntry(
        uint256 const&,
        bool,
        std::shared_ptr<SLE const> const&,
        std::shared_ptr<SLE const> const&);

    bool
    finalize(STTx const&, TER const, XRPAmount const, beast::Journal const&);
};


class NoBadOffers
{
    bool bad_ = false;

public:
    void
    visitEntry(
        uint256 const&,
        bool,
        std::shared_ptr<SLE const> const&,
        std::shared_ptr<SLE const> const&);

    bool
    finalize(STTx const&, TER const, XRPAmount const, beast::Journal const&);

};


class NoZeroEscrow
{
    bool bad_ = false;

public:
    void
    visitEntry(
        uint256 const&,
        bool,
        std::shared_ptr<SLE const> const&,
        std::shared_ptr<SLE const> const&);

    bool
    finalize(STTx const&, TER const, XRPAmount const, beast::Journal const&);

};

using InvariantChecks = std::tuple<
    TransactionFeeCheck,
    AccountRootsNotDeleted,
    LedgerEntryTypesMatch,
    XRPBalanceChecks,
    XRPNotCreated,
    NoXRPTrustLines,
    NoBadOffers,
    NoZeroEscrow
>;


inline
InvariantChecks
getInvariantChecks()
{
    return InvariantChecks{};
}

} 

#endif








