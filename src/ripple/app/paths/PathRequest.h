

#ifndef RIPPLE_APP_PATHS_PATHREQUEST_H_INCLUDED
#define RIPPLE_APP_PATHS_PATHREQUEST_H_INCLUDED

#include <ripple/app/ledger/Ledger.h>
#include <ripple/app/paths/Pathfinder.h>
#include <ripple/app/paths/RippleLineCache.h>
#include <ripple/json/json_value.h>
#include <ripple/net/InfoSub.h>
#include <ripple/protocol/UintTypes.h>
#include <boost/optional.hpp>
#include <map>
#include <mutex>
#include <set>
#include <utility>

namespace ripple {


class RippleLineCache;
class PathRequests;

#define PFR_PJ_INVALID              -1
#define PFR_PJ_NOCHANGE             0
#define PFR_PJ_CHANGE               1

class PathRequest :
    public std::enable_shared_from_this <PathRequest>,
    public CountedObject <PathRequest>
{
public:
    static char const* getCountedObjectName () { return "PathRequest"; }

    using wptr    = std::weak_ptr<PathRequest>;
    using pointer = std::shared_ptr<PathRequest>;
    using ref     = const pointer&;
    using wref    = const wptr&;

public:

    PathRequest (
        Application& app,
        std::shared_ptr <InfoSub> const& subscriber,
        int id,
        PathRequests&,
        beast::Journal journal);

    PathRequest (
        Application& app,
        std::function <void (void)> const& completion,
        Resource::Consumer& consumer,
        int id,
        PathRequests&,
        beast::Journal journal);

    ~PathRequest ();

    bool isNew ();
    bool needsUpdate (bool newOnly, LedgerIndex index);

    void updateComplete ();

    std::pair<bool, Json::Value> doCreate (
        std::shared_ptr<RippleLineCache> const&,
        Json::Value const&);

    Json::Value doClose (Json::Value const&);
    Json::Value doStatus (Json::Value const&);

    Json::Value doUpdate (
        std::shared_ptr<RippleLineCache> const&, bool fast);
    InfoSub::pointer getSubscriber ();
    bool hasCompletion ();

private:
    using ScopedLockType = std::lock_guard <std::recursive_mutex>;

    bool isValid (std::shared_ptr<RippleLineCache> const& crCache);
    void setValid ();

    std::unique_ptr<Pathfinder> const&
    getPathFinder(std::shared_ptr<RippleLineCache> const&,
        hash_map<Currency, std::unique_ptr<Pathfinder>>&, Currency const&,
            STAmount const&, int const);

    
    bool
    findPaths (std::shared_ptr<RippleLineCache> const&, int const, Json::Value&);

    int parseJson (Json::Value const&);

    Application& app_;
    beast::Journal m_journal;

    std::recursive_mutex mLock;

    PathRequests& mOwner;

    std::weak_ptr<InfoSub> wpSubscriber; 
    std::function <void (void)> fCompletion;
    Resource::Consumer& consumer_; 

    Json::Value jvId;
    Json::Value jvStatus; 

    boost::optional<AccountID> raSrcAccount;
    boost::optional<AccountID> raDstAccount;
    STAmount saDstAmount;
    boost::optional<STAmount> saSendMax;

    std::set<Issue> sciSourceCurrencies;
    std::map<Issue, STPathSet> mContext;

    bool convert_all_;

    std::recursive_mutex mIndexLock;
    LedgerIndex mLastIndex;
    bool mInProgress;

    int iLevel;
    bool bLastSuccess;

    int iIdentifier;

    std::chrono::steady_clock::time_point const created_;
    std::chrono::steady_clock::time_point quick_reply_;
    std::chrono::steady_clock::time_point full_reply_;

    static unsigned int const max_paths_ = 4;
};

} 

#endif








