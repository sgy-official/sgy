

#ifndef RIPPLE_APP_LEDGER_INBOUNDTRANSACTIONS_H_INCLUDED
#define RIPPLE_APP_LEDGER_INBOUNDTRANSACTIONS_H_INCLUDED

#include <ripple/overlay/Peer.h>
#include <ripple/shamap/SHAMap.h>
#include <ripple/beast/clock/abstract_clock.h>
#include <ripple/core/Stoppable.h>
#include <memory>

namespace ripple {

class Application;



class InboundTransactions
{
public:
    using clock_type = beast::abstract_clock <std::chrono::steady_clock>;

    InboundTransactions() = default;
    InboundTransactions(InboundTransactions const&) = delete;
    InboundTransactions& operator=(InboundTransactions const&) = delete;

    virtual ~InboundTransactions() = 0;

    
    virtual std::shared_ptr <SHAMap> getSet (
        uint256 const& setHash,
        bool acquire) = 0;

    
    virtual void gotData (uint256 const& setHash,
        std::shared_ptr <Peer>,
        std::shared_ptr <protocol::TMLedgerData>) = 0;

    
    virtual void giveSet (uint256 const& setHash,
        std::shared_ptr <SHAMap> const& set,
        bool acquired) = 0;

    
    virtual void newRound (std::uint32_t seq) = 0;

    virtual Json::Value getInfo() = 0;

    virtual void onStop() = 0;
};

std::unique_ptr <InboundTransactions>
make_InboundTransactions (
    Application& app,
    InboundTransactions::clock_type& clock,
    Stoppable& parent,
    beast::insight::Collector::ptr const& collector,
    std::function
        <void (std::shared_ptr <SHAMap> const&,
            bool)> gotSet);


} 

#endif








