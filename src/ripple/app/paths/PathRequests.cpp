

#include <ripple/app/paths/PathRequests.h>
#include <ripple/app/ledger/LedgerMaster.h>
#include <ripple/app/main/Application.h>
#include <ripple/basics/Log.h>
#include <ripple/core/JobQueue.h>
#include <ripple/net/RPCErr.h>
#include <ripple/protocol/ErrorCodes.h>
#include <ripple/resource/Fees.h>
#include <ripple/protocol/jss.h>
#include <algorithm>

namespace ripple {


std::shared_ptr<RippleLineCache>
PathRequests::getLineCache (
    std::shared_ptr <ReadView const> const& ledger,
    bool authoritative)
{
    ScopedLockType sl (mLock);

    std::uint32_t lineSeq = mLineCache ? mLineCache->getLedger()->seq() : 0;
    std::uint32_t lgrSeq = ledger->seq();

    if ( (lineSeq == 0) ||                                 
         (authoritative && (lgrSeq > lineSeq)) ||          
         (authoritative && ((lgrSeq + 8)  < lineSeq)) ||   
         (lgrSeq > (lineSeq + 8)))                         
    {
        mLineCache = std::make_shared<RippleLineCache> (ledger);
    }
    return mLineCache;
}

void PathRequests::updateAll (std::shared_ptr <ReadView const> const& inLedger,
                              Job::CancelCallback shouldCancel)
{
    auto event =
        app_.getJobQueue().makeLoadEvent(
            jtPATH_FIND, "PathRequest::updateAll");

    std::vector<PathRequest::wptr> requests;
    std::shared_ptr<RippleLineCache> cache;

    {
        ScopedLockType sl (mLock);
        requests = requests_;
        cache = getLineCache (inLedger, true);
    }

    bool newRequests = app_.getLedgerMaster().isNewPathRequest();
    bool mustBreak = false;

    JLOG (mJournal.trace()) <<
        "updateAll seq=" << cache->getLedger()->seq() <<
        ", " << requests.size() << " requests";

    int processed = 0, removed = 0;

    do
    {
        for (auto const& wr : requests)
        {
            if (shouldCancel())
                break;

            auto request = wr.lock ();
            bool remove = true;

            if (request)
            {
                if (!request->needsUpdate (newRequests, cache->getLedger()->seq()))
                    remove = false;
                else
                {
                    if (auto ipSub = request->getSubscriber ())
                    {
                        if (!ipSub->getConsumer ().warn ())
                        {
                            Json::Value update = request->doUpdate (cache, false);
                            request->updateComplete ();
                            update[jss::type] = "path_find";
                            ipSub->send (update, false);
                            remove = false;
                            ++processed;
                        }
                    }
                    else if (request->hasCompletion ())
                    {
                        request->doUpdate (cache, false);
                        request->updateComplete();
                        ++processed;
                    }
                }
            }

            if (remove)
            {
                ScopedLockType sl (mLock);

                auto ret = std::remove_if (
                    requests_.begin(), requests_.end(),
                    [&removed,&request](auto const& wl)
                    {
                        auto r = wl.lock();

                        if (r && r != request)
                            return false;
                        ++removed;
                        return true;
                    });

                requests_.erase (ret, requests_.end());
            }

            mustBreak = !newRequests &&
                app_.getLedgerMaster().isNewPathRequest();

            if (mustBreak)
                break;

        }

        if (mustBreak)
        { 
            newRequests = true;
        }
        else if (newRequests)
        { 
            newRequests = app_.getLedgerMaster().isNewPathRequest();
        }
        else
        { 
            newRequests = app_.getLedgerMaster().isNewPathRequest();
            if (!newRequests)
                break;
        }

        {
            ScopedLockType sl (mLock);

            if (requests_.empty())
                break;
            requests = requests_;
            cache = getLineCache (cache->getLedger(), false);
        }
    }
    while (!shouldCancel ());

    JLOG (mJournal.debug()) <<
        "updateAll complete: " << processed << " processed and " <<
        removed << " removed";
}

void PathRequests::insertPathRequest (
    PathRequest::pointer const& req)
{
    ScopedLockType sl (mLock);

    auto ret = std::find_if (
        requests_.begin(), requests_.end(),
        [](auto const& wl)
        {
            auto r = wl.lock();

            return r && !r->isNew();
        });

    requests_.emplace (ret, req);
}

Json::Value
PathRequests::makePathRequest(
    std::shared_ptr <InfoSub> const& subscriber,
    std::shared_ptr<ReadView const> const& inLedger,
    Json::Value const& requestJson)
{
    auto req = std::make_shared<PathRequest> (
        app_, subscriber, ++mLastIdentifier, *this, mJournal);

    auto result = req->doCreate (
        getLineCache (inLedger, false), requestJson);

    if (result.first)
    {
        subscriber->setPathRequest (req);
        insertPathRequest (req);
        app_.getLedgerMaster().newPathRequest();
    }
    return std::move (result.second);
}

Json::Value
PathRequests::makeLegacyPathRequest(
    PathRequest::pointer& req,
    std::function <void (void)> completion,
    Resource::Consumer& consumer,
    std::shared_ptr<ReadView const> const& inLedger,
    Json::Value const& request)
{
    req = std::make_shared<PathRequest> (
        app_, completion, consumer, ++mLastIdentifier,
            *this, mJournal);

    auto result = req->doCreate (
        getLineCache (inLedger, false), request);

    if (!result.first)
    {
        req.reset();
    }
    else
    {
        insertPathRequest (req);
        if (! app_.getLedgerMaster().newPathRequest())
        {
            result.second = rpcError (rpcTOO_BUSY);
            req.reset();
        }
    }

    return std::move (result.second);
}

Json::Value
PathRequests::doLegacyPathRequest (
        Resource::Consumer& consumer,
        std::shared_ptr<ReadView const> const& inLedger,
        Json::Value const& request)
{
    auto cache = std::make_shared<RippleLineCache> (inLedger);

    auto req = std::make_shared<PathRequest> (app_, []{},
        consumer, ++mLastIdentifier, *this, mJournal);

    auto result = req->doCreate (cache, request);
    if (result.first)
        result.second = req->doUpdate (cache, false);
    return std::move (result.second);
}

} 
























