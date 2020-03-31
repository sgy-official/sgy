

#ifndef RIPPLE_APP_LEDGER_TRANSACTIONACQUIRE_H_INCLUDED
#define RIPPLE_APP_LEDGER_TRANSACTIONACQUIRE_H_INCLUDED

#include <ripple/app/main/Application.h>
#include <ripple/overlay/PeerSet.h>
#include <ripple/shamap/SHAMap.h>

namespace ripple {

class TransactionAcquire
    : public PeerSet
    , public std::enable_shared_from_this <TransactionAcquire>
    , public CountedObject <TransactionAcquire>
{
public:
    static char const* getCountedObjectName () { return "TransactionAcquire"; }

    using pointer = std::shared_ptr<TransactionAcquire>;

public:
    TransactionAcquire (Application& app, uint256 const& hash, clock_type& clock);
    ~TransactionAcquire () = default;

    std::shared_ptr<SHAMap> const& getMap ()
    {
        return mMap;
    }

    SHAMapAddNode takeNodes (const std::list<SHAMapNodeID>& IDs,
                             const std::list< Blob >& data, std::shared_ptr<Peer> const&);

    void init (int startPeers);

    void stillNeed ();

private:

    std::shared_ptr<SHAMap> mMap;
    bool                    mHaveRoot;
    beast::Journal          j_;

    void execute () override;

    void onTimer (bool progress, ScopedLockType& peerSetLock) override;


    void newPeer (std::shared_ptr<Peer> const& peer) override
    {
        trigger (peer);
    }

    void done ();

    void addPeers (int num);

    void trigger (std::shared_ptr<Peer> const&);
    std::weak_ptr<PeerSet> pmDowncast () override;
};

} 

#endif








