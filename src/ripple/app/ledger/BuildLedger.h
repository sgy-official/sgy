

#ifndef RIPPLE_APP_LEDGER_BUILD_LEDGER_H_INCLUDED
#define RIPPLE_APP_LEDGER_BUILD_LEDGER_H_INCLUDED

#include <ripple/ledger/ApplyView.h>
#include <ripple/basics/chrono.h>
#include <ripple/beast/utility/Journal.h>
#include <chrono>
#include <memory>

namespace ripple {

class Application;
class CanonicalTXSet;
class Ledger;
class LedgerReplay;
class SHAMap;



std::shared_ptr<Ledger>
buildLedger(
    std::shared_ptr<Ledger const> const& parent,
    NetClock::time_point closeTime,
    const bool closeTimeCorrect,
    NetClock::duration closeResolution,
    Application& app,
    CanonicalTXSet& txns,
    std::set<TxID>& failedTxs,
    beast::Journal j);


std::shared_ptr<Ledger>
buildLedger(
    LedgerReplay const& replayData,
    ApplyFlags applyFlags,
    Application& app,
    beast::Journal j);

}  
#endif








