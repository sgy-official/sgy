

#ifndef RIPPLE_APP_MISC_NETWORKOPS_H_INCLUDED
#define RIPPLE_APP_MISC_NETWORKOPS_H_INCLUDED

#include <ripple/app/ledger/Ledger.h>
#include <ripple/app/consensus/RCLCxPeerPos.h>
#include <ripple/core/JobQueue.h>
#include <ripple/core/Stoppable.h>
#include <ripple/ledger/ReadView.h>
#include <ripple/net/InfoSub.h>
#include <ripple/protocol/STValidation.h>
#include <boost/asio.hpp>
#include <memory>
#include <deque>
#include <tuple>

#include <ripple/protocol/messages.h>

namespace ripple {


class Peer;
class LedgerMaster;
class Transaction;
class ValidatorKeys;


class NetworkOPs
    : public InfoSub::Source
{
protected:
    explicit NetworkOPs (Stoppable& parent);

public:
    using clock_type = beast::abstract_clock <std::chrono::steady_clock>;

    enum OperatingMode
    {
        omDISCONNECTED  = 0,    
        omCONNECTED     = 1,    
        omSYNCING       = 2,    
        omTRACKING      = 3,    
        omFULL          = 4     
    };

    enum class FailHard : unsigned char
    {
        no,
        yes
    };
    static inline FailHard doFailHard (bool noMeansDont)
    {
        return noMeansDont ? FailHard::yes : FailHard::no;
    }

public:
    ~NetworkOPs () override = default;


    virtual OperatingMode getOperatingMode () const = 0;
    virtual std::string strOperatingMode (bool admin = false) const = 0;


    virtual void submitTransaction (std::shared_ptr<STTx const> const&) = 0;

    
    virtual void processTransaction (std::shared_ptr<Transaction>& transaction,
        bool bUnlimited, bool bLocal, FailHard failType) = 0;


    virtual Json::Value getOwnerInfo (std::shared_ptr<ReadView const> lpLedger,
        AccountID const& account) = 0;


    virtual void getBookPage (
        std::shared_ptr<ReadView const>& lpLedger,
        Book const& book,
        AccountID const& uTakerID,
        bool const bProof,
        unsigned int iLimit,
        Json::Value const& jvMarker,
        Json::Value& jvResult) = 0;


    virtual void processTrustedProposal (RCLCxPeerPos peerPos,
        std::shared_ptr<protocol::TMProposeSet> set) = 0;

    virtual bool recvValidation (STValidation::ref val,
        std::string const& source) = 0;

    virtual void mapComplete (std::shared_ptr<SHAMap> const& map,
        bool fromAcquire) = 0;

    virtual bool beginConsensus (uint256 const& netLCL) = 0;
    virtual void endConsensus () = 0;
    virtual void setStandAlone () = 0;
    virtual void setStateTimer () = 0;

    virtual void setNeedNetworkLedger () = 0;
    virtual void clearNeedNetworkLedger () = 0;
    virtual bool isNeedNetworkLedger () = 0;
    virtual bool isFull () = 0;
    virtual bool isAmendmentBlocked () = 0;
    virtual void setAmendmentBlocked () = 0;
    virtual void consensusViewChange () = 0;

    virtual Json::Value getConsensusInfo () = 0;
    virtual Json::Value getServerInfo (
        bool human, bool admin, bool counters) = 0;
    virtual void clearLedgerFetch () = 0;
    virtual Json::Value getLedgerFetchInfo () = 0;

    
    virtual std::uint32_t acceptLedger (
        boost::optional<std::chrono::milliseconds> consensusDelay = boost::none) = 0;

    virtual uint256 getConsensusLCL () = 0;

    virtual void reportFeeChange () = 0;

    virtual void updateLocalTx (ReadView const& newValidLedger) = 0;
    virtual std::size_t getLocalTxCount () = 0;

    using AccountTx  = std::pair<std::shared_ptr<Transaction>, TxMeta::pointer>;
    using AccountTxs = std::vector<AccountTx>;

    virtual AccountTxs getAccountTxs (
        AccountID const& account,
        std::int32_t minLedger, std::int32_t maxLedger,  bool descending,
        std::uint32_t offset, int limit, bool bUnlimited) = 0;

    virtual AccountTxs getTxsAccount (
        AccountID const& account,
        std::int32_t minLedger, std::int32_t maxLedger, bool forward,
        Json::Value& token, int limit, bool bUnlimited) = 0;

    using txnMetaLedgerType = std::tuple<std::string, std::string, std::uint32_t>;
    using MetaTxsList       = std::vector<txnMetaLedgerType>;

    virtual MetaTxsList getAccountTxsB (AccountID const& account,
        std::int32_t minLedger, std::int32_t maxLedger,  bool descending,
            std::uint32_t offset, int limit, bool bUnlimited) = 0;

    virtual MetaTxsList getTxsAccountB (AccountID const& account,
        std::int32_t minLedger, std::int32_t maxLedger,  bool forward,
        Json::Value& token, int limit, bool bUnlimited) = 0;

    virtual void pubLedger (
        std::shared_ptr<ReadView const> const& lpAccepted) = 0;
    virtual void pubProposedTransaction (
        std::shared_ptr<ReadView const> const& lpCurrent,
        std::shared_ptr<STTx const> const& stTxn, TER terResult) = 0;
    virtual void pubValidation (STValidation::ref val) = 0;
};


std::unique_ptr<NetworkOPs>
make_NetworkOPs (Application& app, NetworkOPs::clock_type& clock,
    bool standalone, std::size_t minPeerCount, bool start_valid,
    JobQueue& job_queue, LedgerMaster& ledgerMaster, Stoppable& parent,
    ValidatorKeys const & validatorKeys, boost::asio::io_service& io_svc,
    beast::Journal journal);

} 

#endif








