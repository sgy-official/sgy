

#ifndef RIPPLE_APP_LEDGER_INBOUNDLEDGER_H_INCLUDED
#define RIPPLE_APP_LEDGER_INBOUNDLEDGER_H_INCLUDED

#include <ripple/app/main/Application.h>
#include <ripple/app/ledger/Ledger.h>
#include <ripple/overlay/PeerSet.h>
#include <ripple/basics/CountedObject.h>
#include <mutex>
#include <set>
#include <utility>

namespace ripple {

class InboundLedger
    : public PeerSet
    , public std::enable_shared_from_this <InboundLedger>
    , public CountedObject <InboundLedger>
{
public:
    static char const* getCountedObjectName () { return "InboundLedger"; }

    using PeerDataPairType = std::pair<
        std::weak_ptr<Peer>,
        std::shared_ptr<protocol::TMLedgerData>>;

    enum class Reason
    {
        HISTORY,  
        SHARD,    
        GENERIC,  
        CONSENSUS 
    };

    InboundLedger(Application& app, uint256 const& hash,
        std::uint32_t seq, Reason reason, clock_type&);

    ~InboundLedger ();

    void execute () override;

    void update (std::uint32_t seq);

    std::shared_ptr<Ledger const>
    getLedger() const
    {
        return mLedger;
    }

    std::uint32_t getSeq () const
    {
        return mSeq;
    }

    Reason
    getReason() const
    {
        return mReason;
    }

    bool checkLocal ();
    void init (ScopedLockType& collectionLock);

    bool
    gotData(std::weak_ptr<Peer>,
        std::shared_ptr<protocol::TMLedgerData> const&);

    using neededHash_t =
        std::pair <protocol::TMGetObjectByHash::ObjectType, uint256>;

    
    Json::Value getJson (int);

    void runData ();

    static
    LedgerInfo
    deserializeHeader(Slice data, bool hasPrefix);

private:
    enum class TriggerReason
    {
        added,
        reply,
        timeout
    };

    void filterNodes (
        std::vector<std::pair<SHAMapNodeID, uint256>>& nodes,
        TriggerReason reason);

    void trigger (std::shared_ptr<Peer> const&, TriggerReason);

    std::vector<neededHash_t> getNeededHashes ();

    void addPeers ();
    void tryDB (Family& f);

    void done ();

    void onTimer (bool progress, ScopedLockType& peerSetLock) override;

    void newPeer (std::shared_ptr<Peer> const& peer) override
    {
        if (mReason != Reason::HISTORY)
            trigger (peer, TriggerReason::added);
    }

    std::weak_ptr <PeerSet> pmDowncast () override;

    int processData (std::shared_ptr<Peer> peer, protocol::TMLedgerData& data);

    bool takeHeader (std::string const& data);
    bool takeTxNode (const std::vector<SHAMapNodeID>& IDs,
                     const std::vector<Blob>& data,
                     SHAMapAddNode&);
    bool takeTxRootNode (Slice const& data, SHAMapAddNode&);

    bool takeAsNode (const std::vector<SHAMapNodeID>& IDs,
                     const std::vector<Blob>& data,
                     SHAMapAddNode&);
    bool takeAsRootNode (Slice const& data, SHAMapAddNode&);

    std::vector<uint256>
    neededTxHashes (
        int max, SHAMapSyncFilter* filter) const;

    std::vector<uint256>
    neededStateHashes (
        int max, SHAMapSyncFilter* filter) const;

    std::shared_ptr<Ledger> mLedger;
    bool mHaveHeader;
    bool mHaveState;
    bool mHaveTransactions;
    bool mSignaled;
    bool mByHash;
    std::uint32_t mSeq;
    Reason const mReason;

    std::set <uint256> mRecentNodes;

    SHAMapAddNode mStats;

    std::mutex mReceivedDataLock;
    std::vector <PeerDataPairType> mReceivedData;
    bool mReceiveDispatched;
};

} 

#endif








