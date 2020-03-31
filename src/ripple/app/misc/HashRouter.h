

#ifndef RIPPLE_APP_MISC_HASHROUTER_H_INCLUDED
#define RIPPLE_APP_MISC_HASHROUTER_H_INCLUDED

#include <ripple/basics/base_uint.h>
#include <ripple/basics/chrono.h>
#include <ripple/basics/CountedObject.h>
#include <ripple/basics/UnorderedContainers.h>
#include <ripple/beast/container/aged_unordered_map.h>
#include <boost/optional.hpp>

namespace ripple {

#define SF_BAD          0x02    
#define SF_SAVED        0x04
#define SF_TRUSTED      0x10    
#define SF_PRIVATE1     0x0100
#define SF_PRIVATE2     0x0200
#define SF_PRIVATE3     0x0400
#define SF_PRIVATE4     0x0800
#define SF_PRIVATE5     0x1000
#define SF_PRIVATE6     0x2000


class HashRouter
{
public:
    using PeerShortID = std::uint32_t;

private:
    
    class Entry : public CountedObject <Entry>
    {
    public:
        static char const* getCountedObjectName () { return "HashRouterEntry"; }

        Entry ()
        {
        }

        void addPeer (PeerShortID peer)
        {
            if (peer != 0)
                peers_.insert (peer);
        }

        int getFlags (void) const
        {
            return flags_;
        }

        void setFlags (int flagsToSet)
        {
            flags_ |= flagsToSet;
        }

        
        std::set<PeerShortID> releasePeerSet()
        {
            return std::move(peers_);
        }

        
        bool shouldRelay (Stopwatch::time_point const& now,
            std::chrono::seconds holdTime)
        {
            if (relayed_ && *relayed_ + holdTime > now)
                return false;
            relayed_.emplace(now);
            return true;
        }

        
        bool shouldRecover(std::uint32_t limit)
        {
            return ++recoveries_ % limit != 0;
        }

        bool shouldProcess(Stopwatch::time_point now, std::chrono::seconds interval)
        {
             if (processed_ && ((*processed_ + interval) > now))
                 return false;
             processed_.emplace (now);
             return true;
        }

    private:
        int flags_ = 0;
        std::set <PeerShortID> peers_;
        boost::optional<Stopwatch::time_point> relayed_;
        boost::optional<Stopwatch::time_point> processed_;
        std::uint32_t recoveries_ = 0;
    };

public:
    static inline std::chrono::seconds getDefaultHoldTime ()
    {
        using namespace std::chrono;

        return 300s;
    }

    static inline std::uint32_t getDefaultRecoverLimit()
    {
        return 1;
    }

    HashRouter (Stopwatch& clock, std::chrono::seconds entryHoldTimeInSeconds,
        std::uint32_t recoverLimit)
        : suppressionMap_(clock)
        , holdTime_ (entryHoldTimeInSeconds)
        , recoverLimit_ (recoverLimit + 1u)
    {
    }

    HashRouter& operator= (HashRouter const&) = delete;

    virtual ~HashRouter() = default;

    void addSuppression(uint256 const& key);

    bool addSuppressionPeer (uint256 const& key, PeerShortID peer);

    bool addSuppressionPeer (uint256 const& key, PeerShortID peer,
                             int& flags);

    bool shouldProcess (uint256 const& key, PeerShortID peer, int& flags,
        std::chrono::seconds tx_interval);

    
    bool setFlags (uint256 const& key, int flags);

    int getFlags (uint256 const& key);

    
    boost::optional<std::set<PeerShortID>> shouldRelay(uint256 const& key);

    
    bool shouldRecover(uint256 const& key);

private:
    std::pair<Entry&, bool> emplace (uint256 const&);

    std::mutex mutable mutex_;

    beast::aged_unordered_map<uint256, Entry, Stopwatch::clock_type,
        hardened_hash<strong_hash>> suppressionMap_;

    std::chrono::seconds const holdTime_;

    std::uint32_t const recoverLimit_;
};

} 

#endif








