

#ifndef RIPPLE_APP_PEERS_PEERSET_H_INCLUDED
#define RIPPLE_APP_PEERS_PEERSET_H_INCLUDED

#include <ripple/app/main/Application.h>
#include <ripple/beast/clock/abstract_clock.h>
#include <ripple/beast/utility/Journal.h>
#include <ripple/overlay/Peer.h>
#include <boost/asio/basic_waitable_timer.hpp>
#include <mutex>
#include <set>

namespace ripple {


class PeerSet
{
public:
    using clock_type = beast::abstract_clock <std::chrono::steady_clock>;

    
    uint256 const& getHash () const
    {
        return mHash;
    }

    
    bool isComplete () const
    {
        return mComplete;
    }

    
    bool isFailed () const
    {
        return mFailed;
    }

    
    int getTimeouts () const
    {
        return mTimeouts;
    }

    bool isActive ();

    
    void progress ()
    {
        mProgress = true;
    }

    void touch ()
    {
        mLastAction = m_clock.now();
    }

    clock_type::time_point getLastAction () const
    {
        return mLastAction;
    }

    
    bool insert (std::shared_ptr<Peer> const&);

    virtual bool isDone () const
    {
        return mComplete || mFailed;
    }

    Application&
    app()
    {
        return app_;
    }

protected:
    using ScopedLockType = std::unique_lock <std::recursive_mutex>;

    PeerSet (Application& app, uint256 const& hash, std::chrono::milliseconds interval,
        clock_type& clock, beast::Journal journal);

    virtual ~PeerSet() = 0;

    virtual void newPeer (std::shared_ptr<Peer> const&) = 0;

    virtual void onTimer (bool progress, ScopedLockType&) = 0;

    virtual void execute () = 0;

    virtual std::weak_ptr<PeerSet> pmDowncast () = 0;

    bool isProgress ()
    {
        return mProgress;
    }

    void setComplete ()
    {
        mComplete = true;
    }
    void setFailed ()
    {
        mFailed = true;
    }

    void invokeOnTimer ();

    void sendRequest (const protocol::TMGetLedger& message);

    void sendRequest (const protocol::TMGetLedger& message, std::shared_ptr<Peer> const& peer);

    void setTimer ();

    std::size_t getPeerCount () const;

protected:
    Application& app_;
    beast::Journal m_journal;
    clock_type& m_clock;

    std::recursive_mutex mLock;

    uint256 mHash;
    std::chrono::milliseconds mTimerInterval;
    int mTimeouts;
    bool mComplete;
    bool mFailed;
    clock_type::time_point mLastAction;
    bool mProgress;

    boost::asio::basic_waitable_timer<std::chrono::steady_clock> mTimer;

    std::set <Peer::id_t> mPeers;
};

} 

#endif








