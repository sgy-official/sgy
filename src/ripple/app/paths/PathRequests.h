

#ifndef RIPPLE_APP_PATHS_PATHREQUESTS_H_INCLUDED
#define RIPPLE_APP_PATHS_PATHREQUESTS_H_INCLUDED

#include <ripple/app/main/Application.h>
#include <ripple/app/paths/PathRequest.h>
#include <ripple/app/paths/RippleLineCache.h>
#include <ripple/core/Job.h>
#include <atomic>
#include <mutex>
#include <vector>

namespace ripple {

class PathRequests
{
public:
    
    PathRequests (Application& app,
            beast::Journal journal, beast::insight::Collector::ptr const& collector)
        : app_ (app)
        , mJournal (journal)
        , mLastIdentifier (0)
    {
        mFast = collector->make_event ("pathfind_fast");
        mFull = collector->make_event ("pathfind_full");
    }

    
    void updateAll (std::shared_ptr<ReadView const> const& ledger,
                    Job::CancelCallback shouldCancel);

    std::shared_ptr<RippleLineCache> getLineCache (
        std::shared_ptr <ReadView const> const& ledger, bool authoritative);

    Json::Value makePathRequest (
        std::shared_ptr <InfoSub> const& subscriber,
        std::shared_ptr<ReadView const> const& ledger,
        Json::Value const& request);

    Json::Value makeLegacyPathRequest (
        PathRequest::pointer& req,
        std::function <void (void)> completion,
        Resource::Consumer& consumer,
        std::shared_ptr<ReadView const> const& inLedger,
        Json::Value const& request);

    Json::Value doLegacyPathRequest (
        Resource::Consumer& consumer,
        std::shared_ptr<ReadView const> const& inLedger,
        Json::Value const& request);

    void reportFast (std::chrono::milliseconds ms)
    {
        mFast.notify (ms);
    }

    void reportFull (std::chrono::milliseconds ms)
    {
        mFull.notify (ms);
    }

private:
    void insertPathRequest (PathRequest::pointer const&);

    Application& app_;
    beast::Journal                   mJournal;

    beast::insight::Event            mFast;
    beast::insight::Event            mFull;

    std::vector<PathRequest::wptr> requests_;

    std::shared_ptr<RippleLineCache>         mLineCache;

    std::atomic<int>                 mLastIdentifier;

    using ScopedLockType = std::lock_guard <std::recursive_mutex>;
    std::recursive_mutex mLock;

};

} 

#endif








