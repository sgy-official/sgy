

#ifndef RIPPLE_APP_MISC_IMPL_ACCOUNTTXPAGING_H_INCLUDED
#define RIPPLE_APP_MISC_IMPL_ACCOUNTTXPAGING_H_INCLUDED

#include <ripple/core/DatabaseCon.h>
#include <ripple/app/misc/NetworkOPs.h>
#include <cstdint>
#include <string>
#include <utility>



namespace ripple {

void
convertBlobsToTxResult (
    NetworkOPs::AccountTxs& to,
    std::uint32_t ledger_index,
    std::string const& status,
    Blob const& rawTxn,
    Blob const& rawMeta,
    Application& app);

void
saveLedgerAsync (Application& app, std::uint32_t seq);

void
accountTxPage (
    DatabaseCon& database,
    AccountIDCache const& idCache,
    std::function<void (std::uint32_t)> const& onUnsavedLedger,
    std::function<void (std::uint32_t,
                        std::string const&,
                        Blob const&,
                        Blob const&)> const&,
    AccountID const& account,
    std::int32_t minLedger,
    std::int32_t maxLedger,
    bool forward,
    Json::Value& token,
    int limit,
    bool bAdmin,
    std::uint32_t pageLength);

}

#endif








