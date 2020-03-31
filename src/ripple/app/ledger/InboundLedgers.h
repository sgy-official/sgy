

#ifndef RIPPLE_APP_LEDGER_INBOUNDLEDGERS_H_INCLUDED
#define RIPPLE_APP_LEDGER_INBOUNDLEDGERS_H_INCLUDED

#include <ripple/app/ledger/InboundLedger.h>
#include <ripple/protocol/RippleLedgerHash.h>
#include <ripple/core/Stoppable.h>
#include <memory>

namespace ripple {


class InboundLedgers
{
public:
    using clock_type = beast::abstract_clock <std::chrono::steady_clock>;

    virtual ~InboundLedgers() = 0;

    virtual
    std::shared_ptr<Ledger const>
    acquire (uint256 const& hash,
        std::uint32_t seq, InboundLedger::Reason) = 0;

    virtual std::shared_ptr<InboundLedger> find (LedgerHash const& hash) = 0;

    virtual bool gotLedgerData (LedgerHash const& ledgerHash,
        std::shared_ptr<Peer>,
        std::shared_ptr <protocol::TMLedgerData>) = 0;

    virtual void doLedgerData (LedgerHash hash) = 0;

    virtual void gotStaleData (
        std::shared_ptr <protocol::TMLedgerData> packet) = 0;

    virtual int getFetchCount (int& timeoutCount) = 0;

    virtual void logFailure (uint256 const& h, std::uint32_t seq) = 0;

    virtual bool isFailure (uint256 const& h) = 0;

    virtual void clearFailures() = 0;

    virtual Json::Value getInfo() = 0;

    
    virtual std::size_t fetchRate() = 0;

    
    virtual void onLedgerFetched() = 0;

    virtual void gotFetchPack () = 0;
    virtual void sweep () = 0;

    virtual void onStop() = 0;
};

std::unique_ptr<InboundLedgers>
make_InboundLedgers (Application& app,
    InboundLedgers::clock_type& clock, Stoppable& parent,
    beast::insight::Collector::ptr const& collector);


} 

#endif








