

#ifndef RIPPLE_APP_CONSENSUS_RCLCXLEDGER_H_INCLUDED
#define RIPPLE_APP_CONSENSUS_RCLCXLEDGER_H_INCLUDED

#include <ripple/app/ledger/Ledger.h>
#include <ripple/app/ledger/LedgerToJson.h>
#include <ripple/ledger/ReadView.h>
#include <ripple/protocol/RippleLedgerHash.h>
#include <memory>

namespace ripple {


class RCLCxLedger
{
public:
    using ID = LedgerHash;
    using Seq = LedgerIndex;

    
    RCLCxLedger() = default;

    
    RCLCxLedger(std::shared_ptr<Ledger const> const& l) : ledger_{l}
    {
    }

    Seq const&
    seq() const
    {
        return ledger_->info().seq;
    }

    ID const&
    id() const
    {
        return ledger_->info().hash;
    }

    ID const&
    parentID() const
    {
        return ledger_->info().parentHash;
    }

    NetClock::duration
    closeTimeResolution() const
    {
        return ledger_->info().closeTimeResolution;
    }

    bool
    closeAgree() const
    {
        return ripple::getCloseAgree(ledger_->info());
    }

    NetClock::time_point
    closeTime() const
    {
        return ledger_->info().closeTime;
    }

    NetClock::time_point
    parentCloseTime() const
    {
        return ledger_->info().parentCloseTime;
    }

    Json::Value
    getJson() const
    {
        return ripple::getJson(*ledger_);
    }

    
    std::shared_ptr<Ledger const> ledger_;
};
}
#endif








