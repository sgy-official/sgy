

#ifndef RIPPLE_APP_LEDGER_LEDGERHISTORY_H_INCLUDED
#define RIPPLE_APP_LEDGER_LEDGERHISTORY_H_INCLUDED

#include <ripple/app/ledger/Ledger.h>
#include <ripple/app/main/Application.h>
#include <ripple/protocol/RippleLedgerHash.h>
#include <ripple/beast/insight/Collector.h>
#include <ripple/beast/insight/Event.h>

namespace ripple {



class LedgerHistory
{
public:
    LedgerHistory (beast::insight::Collector::ptr const& collector,
        Application& app);

    
    bool insert (std::shared_ptr<Ledger const> ledger,
        bool validated);

    
    float getCacheHitRate ()
    {
        return m_ledgers_by_hash.getHitRate ();
    }

    
    std::shared_ptr<Ledger const>
    getLedgerBySeq (LedgerIndex ledgerIndex);

    
    std::shared_ptr<Ledger const>
    getLedgerByHash (LedgerHash const& ledgerHash);

    
    LedgerHash getLedgerHash (LedgerIndex ledgerIndex);

    
    void tune (int size, std::chrono::seconds age);

    
    void sweep ()
    {
        m_ledgers_by_hash.sweep ();
        m_consensus_validated.sweep ();
    }

    
    void builtLedger (
        std::shared_ptr<Ledger const> const&,
        uint256 const& consensusHash,
        Json::Value);

    
    void validatedLedger (
        std::shared_ptr<Ledger const> const&,
        boost::optional<uint256> const& consensusHash);

    
    bool fixIndex(LedgerIndex ledgerIndex, LedgerHash const& ledgerHash);

    void clearLedgerCachePrior (LedgerIndex seq);

private:

    
    void
    handleMismatch(
        LedgerHash const& built,
        LedgerHash const& valid,
        boost::optional<uint256> const& builtConsensusHash,
        boost::optional<uint256> const& validatedConsensusHash,
        Json::Value const& consensus);

    Application& app_;
    beast::insight::Collector::ptr collector_;
    beast::insight::Counter mismatch_counter_;

    using LedgersByHash = TaggedCache <LedgerHash, Ledger const>;

    LedgersByHash m_ledgers_by_hash;

    struct cv_entry
    {
        boost::optional<LedgerHash> built;
        boost::optional<LedgerHash> validated;
        boost::optional<uint256> builtConsensusHash;
        boost::optional<uint256> validatedConsensusHash;
        boost::optional<Json::Value> consensus;
    };
    using ConsensusValidated = TaggedCache <LedgerIndex, cv_entry>;
    ConsensusValidated m_consensus_validated;


    std::map <LedgerIndex, LedgerHash> mLedgersByIndex; 

    beast::Journal j_;
};

} 

#endif








