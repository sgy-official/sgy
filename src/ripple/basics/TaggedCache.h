

#ifndef RIPPLE_BASICS_TAGGEDCACHE_H_INCLUDED
#define RIPPLE_BASICS_TAGGEDCACHE_H_INCLUDED

#include <ripple/basics/hardened_hash.h>
#include <ripple/basics/Log.h>
#include <ripple/basics/UnorderedContainers.h>
#include <ripple/beast/clock/abstract_clock.h>
#include <ripple/beast/insight/Insight.h>
#include <functional>
#include <mutex>
#include <vector>

namespace ripple {

struct TaggedCacheLog;


template <
    class Key,
    class T,
    class Hash = hardened_hash <>,
    class KeyEqual = std::equal_to <Key>,
    class Mutex = std::recursive_mutex
>
class TaggedCache
{
public:
    using mutex_type = Mutex;
    using ScopedLockType = std::unique_lock <mutex_type>;
    using lock_guard = std::lock_guard <mutex_type>;
    using key_type = Key;
    using mapped_type = T;
    using weak_mapped_ptr = std::weak_ptr <mapped_type>;
    using mapped_ptr = std::shared_ptr <mapped_type>;
    using clock_type = beast::abstract_clock <std::chrono::steady_clock>;

public:
    TaggedCache (std::string const& name, int size,
        clock_type::duration expiration, clock_type& clock, beast::Journal journal,
            beast::insight::Collector::ptr const& collector = beast::insight::NullCollector::New ())
        : m_journal (journal)
        , m_clock (clock)
        , m_stats (name,
            std::bind (&TaggedCache::collect_metrics, this),
                collector)
        , m_name (name)
        , m_target_size (size)
        , m_target_age (expiration)
        , m_cache_count (0)
        , m_hits (0)
        , m_misses (0)
    {
    }

public:
    
    clock_type& clock ()
    {
        return m_clock;
    }

    int getTargetSize () const
    {
        lock_guard lock (m_mutex);
        return m_target_size;
    }

    void setTargetSize (int s)
    {
        lock_guard lock (m_mutex);
        m_target_size = s;

        if (s > 0)
            m_cache.rehash (static_cast<std::size_t> ((s + (s >> 2)) / m_cache.max_load_factor () + 1));

        JLOG(m_journal.debug()) <<
            m_name << " target size set to " << s;
    }

    clock_type::duration getTargetAge () const
    {
        lock_guard lock (m_mutex);
        return m_target_age;
    }

    void setTargetAge (clock_type::duration s)
    {
        lock_guard lock (m_mutex);
        m_target_age = s;
        JLOG(m_journal.debug()) <<
            m_name << " target age set to " << m_target_age.count();
    }

    int getCacheSize () const
    {
        lock_guard lock (m_mutex);
        return m_cache_count;
    }

    int getTrackSize () const
    {
        lock_guard lock (m_mutex);
        return m_cache.size ();
    }

    float getHitRate ()
    {
        lock_guard lock (m_mutex);
        auto const total = static_cast<float> (m_hits + m_misses);
        return m_hits * (100.0f / std::max (1.0f, total));
    }

    void clear ()
    {
        lock_guard lock (m_mutex);
        m_cache.clear ();
        m_cache_count = 0;
    }

    void reset ()
    {
        lock_guard lock (m_mutex);
        m_cache.clear();
        m_cache_count = 0;
        m_hits = 0;
        m_misses = 0;
    }

    void sweep ()
    {
        int cacheRemovals = 0;
        int mapRemovals = 0;
        int cc = 0;

        std::vector <mapped_ptr> stuffToSweep;

        {
            clock_type::time_point const now (m_clock.now());
            clock_type::time_point when_expire;

            lock_guard lock (m_mutex);

            if (m_target_size == 0 ||
                (static_cast<int> (m_cache.size ()) <= m_target_size))
            {
                when_expire = now - m_target_age;
            }
            else
            {
                when_expire = now - m_target_age*m_target_size/m_cache.size();

                clock_type::duration const minimumAge (
                    std::chrono::seconds (1));
                if (when_expire > (now - minimumAge))
                    when_expire = now - minimumAge;

                JLOG(m_journal.trace()) <<
                    m_name << " is growing fast " << m_cache.size () << " of " << m_target_size <<
                        " aging at " << (now - when_expire).count() << " of " << m_target_age.count();
            }

            stuffToSweep.reserve (m_cache.size ());

            cache_iterator cit = m_cache.begin ();

            while (cit != m_cache.end ())
            {
                if (cit->second.isWeak ())
                {
                    if (cit->second.isExpired ())
                    {
                        ++mapRemovals;
                        cit = m_cache.erase (cit);
                    }
                    else
                    {
                        ++cit;
                    }
                }
                else if (cit->second.last_access <= when_expire)
                {
                    --m_cache_count;
                    ++cacheRemovals;
                    if (cit->second.ptr.unique ())
                    {
                        stuffToSweep.push_back (cit->second.ptr);
                        ++mapRemovals;
                        cit = m_cache.erase (cit);
                    }
                    else
                    {
                        cit->second.ptr.reset ();
                        ++cit;
                    }
                }
                else
                {
                    ++cc;
                    ++cit;
                }
            }
        }

        if (mapRemovals || cacheRemovals)
        {
            JLOG(m_journal.trace()) <<
                m_name << ": cache = " << m_cache.size () <<
                "-" << cacheRemovals << ", map-=" << mapRemovals;
        }

    }

    bool del (const key_type& key, bool valid)
    {
        lock_guard lock (m_mutex);

        cache_iterator cit = m_cache.find (key);

        if (cit == m_cache.end ())
            return false;

        Entry& entry = cit->second;

        bool ret = false;

        if (entry.isCached ())
        {
            --m_cache_count;
            entry.ptr.reset ();
            ret = true;
        }

        if (!valid || entry.isExpired ())
            m_cache.erase (cit);

        return ret;
    }

    
    bool canonicalize (const key_type& key, std::shared_ptr<T>& data, bool replace = false)
    {
        lock_guard lock (m_mutex);

        cache_iterator cit = m_cache.find (key);

        if (cit == m_cache.end ())
        {
            m_cache.emplace (std::piecewise_construct,
                std::forward_as_tuple(key),
                std::forward_as_tuple(m_clock.now(), data));
            ++m_cache_count;
            return false;
        }

        Entry& entry = cit->second;
        entry.touch (m_clock.now());

        if (entry.isCached ())
        {
            if (replace)
            {
                entry.ptr = data;
                entry.weak_ptr = data;
            }
            else
            {
                data = entry.ptr;
            }

            return true;
        }

        mapped_ptr cachedData = entry.lock ();

        if (cachedData)
        {
            if (replace)
            {
                entry.ptr = data;
                entry.weak_ptr = data;
            }
            else
            {
                entry.ptr = cachedData;
                data = cachedData;
            }

            ++m_cache_count;
            return true;
        }

        entry.ptr = data;
        entry.weak_ptr = data;
        ++m_cache_count;

        return false;
    }

    std::shared_ptr<T> fetch (const key_type& key)
    {
        lock_guard lock (m_mutex);

        cache_iterator cit = m_cache.find (key);

        if (cit == m_cache.end ())
        {
            ++m_misses;
            return mapped_ptr ();
        }

        Entry& entry = cit->second;
        entry.touch (m_clock.now());

        if (entry.isCached ())
        {
            ++m_hits;
            return entry.ptr;
        }

        entry.ptr = entry.lock ();

        if (entry.isCached ())
        {
            ++m_cache_count;
            return entry.ptr;
        }

        m_cache.erase (cit);
        ++m_misses;
        return mapped_ptr ();
    }

    
    bool insert (key_type const& key, T const& value)
    {
        mapped_ptr p (std::make_shared <T> (
            std::cref (value)));
        return canonicalize (key, p);
    }

    bool retrieve (const key_type& key, T& data)
    {
        mapped_ptr entry = fetch (key);

        if (!entry)
            return false;

        data = *entry;
        return true;
    }

    
    bool refreshIfPresent (const key_type& key)
    {
        bool found = false;

        lock_guard lock (m_mutex);

        cache_iterator cit = m_cache.find (key);

        if (cit != m_cache.end ())
        {
            Entry& entry = cit->second;

            if (! entry.isCached ())
            {
                entry.ptr = entry.lock ();

                if (entry.isCached ())
                {
                    ++m_cache_count;
                    entry.touch (m_clock.now());
                    found = true;
                }
                else
                {
                    m_cache.erase (cit);
                }
            }
            else
            {
                entry.touch (m_clock.now());
                found = true;
            }
        }
        else
        {
        }

        return found;
    }

    mutex_type& peekMutex ()
    {
        return m_mutex;
    }

    std::vector <key_type> getKeys () const
    {
        std::vector <key_type> v;

        {
            lock_guard lock (m_mutex);
            v.reserve (m_cache.size());
            for (auto const& _ : m_cache)
                v.push_back (_.first);
        }

        return v;
    }

private:
    void collect_metrics ()
    {
        m_stats.size.set (getCacheSize ());

        {
            beast::insight::Gauge::value_type hit_rate (0);
            {
                lock_guard lock (m_mutex);
                auto const total (m_hits + m_misses);
                if (total != 0)
                    hit_rate = (m_hits * 100) / total;
            }
            m_stats.hit_rate.set (hit_rate);
        }
    }

private:
    struct Stats
    {
        template <class Handler>
        Stats (std::string const& prefix, Handler const& handler,
            beast::insight::Collector::ptr const& collector)
            : hook (collector->make_hook (handler))
            , size (collector->make_gauge (prefix, "size"))
            , hit_rate (collector->make_gauge (prefix, "hit_rate"))
            { }

        beast::insight::Hook hook;
        beast::insight::Gauge size;
        beast::insight::Gauge hit_rate;
    };

    class Entry
    {
    public:
        mapped_ptr ptr;
        weak_mapped_ptr weak_ptr;
        clock_type::time_point last_access;

        Entry (clock_type::time_point const& last_access_,
            mapped_ptr const& ptr_)
            : ptr (ptr_)
            , weak_ptr (ptr_)
            , last_access (last_access_)
        {
        }

        bool isWeak () const { return ptr == nullptr; }
        bool isCached () const { return ptr != nullptr; }
        bool isExpired () const { return weak_ptr.expired (); }
        mapped_ptr lock () { return weak_ptr.lock (); }
        void touch (clock_type::time_point const& now) { last_access = now; }
    };

    using cache_type = hardened_hash_map <key_type, Entry, Hash, KeyEqual>;
    using cache_iterator = typename cache_type::iterator;

    beast::Journal m_journal;
    clock_type& m_clock;
    Stats m_stats;

    mutex_type mutable m_mutex;

    std::string m_name;

    int m_target_size;

    clock_type::duration m_target_age;

    int m_cache_count;
    cache_type m_cache;  
    std::uint64_t m_hits;
    std::uint64_t m_misses;
};

}

#endif








