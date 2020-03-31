

#ifndef RIPPLE_BASICS_KEYCACHE_H_INCLUDED
#define RIPPLE_BASICS_KEYCACHE_H_INCLUDED

#include <ripple/basics/hardened_hash.h>
#include <ripple/basics/UnorderedContainers.h>
#include <ripple/beast/clock/abstract_clock.h>
#include <ripple/beast/insight/Insight.h>
#include <mutex>

namespace ripple {


template <
    class Key,
    class Hash = hardened_hash <>,
    class KeyEqual = std::equal_to <Key>,
    class Mutex = std::mutex
>
class KeyCache
{
public:
    using key_type = Key;
    using clock_type = beast::abstract_clock <std::chrono::steady_clock>;

private:
    struct Stats
    {
        template <class Handler>
        Stats (std::string const& prefix, Handler const& handler,
            beast::insight::Collector::ptr const& collector)
            : hook (collector->make_hook (handler))
            , size (collector->make_gauge (prefix, "size"))
            , hit_rate (collector->make_gauge (prefix, "hit_rate"))
            , hits (0)
            , misses (0)
            { }

        beast::insight::Hook hook;
        beast::insight::Gauge size;
        beast::insight::Gauge hit_rate;

        std::size_t hits;
        std::size_t misses;
    };

    struct Entry
    {
        explicit Entry (clock_type::time_point const& last_access_)
            : last_access (last_access_)
        {
        }

        clock_type::time_point last_access;
    };

    using map_type = hardened_hash_map <key_type, Entry, Hash, KeyEqual>;
    using iterator = typename map_type::iterator;
    using lock_guard = std::lock_guard <Mutex>;

public:
    using size_type = typename map_type::size_type;

private:
    Mutex mutable m_mutex;
    map_type m_map;
    Stats mutable m_stats;
    clock_type& m_clock;
    std::string const m_name;
    size_type m_target_size;
    clock_type::duration m_target_age;

public:
    
    KeyCache (std::string const& name, clock_type& clock,
        beast::insight::Collector::ptr const& collector, size_type target_size = 0,
            std::chrono::seconds expiration = std::chrono::minutes{2})
        : m_stats (name,
            std::bind (&KeyCache::collect_metrics, this),
                collector)
        , m_clock (clock)
        , m_name (name)
        , m_target_size (target_size)
        , m_target_age (expiration)
    {
    }

    KeyCache (std::string const& name, clock_type& clock,
        size_type target_size = 0,
        std::chrono::seconds expiration = std::chrono::minutes{2})
        : m_stats (name,
            std::bind (&KeyCache::collect_metrics, this),
                beast::insight::NullCollector::New ())
        , m_clock (clock)
        , m_name (name)
        , m_target_size (target_size)
        , m_target_age (expiration)
    {
    }


    
    std::string const& name () const
    {
        return m_name;
    }

    
    clock_type& clock ()
    {
        return m_clock;
    }

    
    size_type size () const
    {
        lock_guard lock (m_mutex);
        return m_map.size ();
    }

    
    void clear ()
    {
        lock_guard lock (m_mutex);
        m_map.clear ();
    }

    void reset ()
    {
        lock_guard lock(m_mutex);
        m_map.clear();
        m_stats.hits = 0;
        m_stats.misses = 0;
    }

    void setTargetSize (size_type s)
    {
        lock_guard lock (m_mutex);
        m_target_size = s;
    }

    void setTargetAge (std::chrono::seconds s)
    {
        lock_guard lock (m_mutex);
        m_target_age = s;
    }

    
    template <class KeyComparable>
    bool exists (KeyComparable const& key) const
    {
        lock_guard lock (m_mutex);
        typename map_type::const_iterator const iter (m_map.find (key));
        if (iter != m_map.end ())
        {
            ++m_stats.hits;
            return true;
        }
        ++m_stats.misses;
        return false;
    }

    
    bool insert (Key const& key)
    {
        lock_guard lock (m_mutex);
        clock_type::time_point const now (m_clock.now ());
        std::pair <iterator, bool> result (m_map.emplace (
            std::piecewise_construct, std::forward_as_tuple (key),
                std::forward_as_tuple (now)));
        if (! result.second)
        {
            result.first->second.last_access = now;
            return false;
        }
        return true;
    }

    
    template <class KeyComparable>
    bool touch_if_exists (KeyComparable const& key)
    {
        lock_guard lock (m_mutex);
        iterator const iter (m_map.find (key));
        if (iter == m_map.end ())
        {
            ++m_stats.misses;
            return false;
        }
        iter->second.last_access = m_clock.now ();
        ++m_stats.hits;
        return true;
    }

    
    bool erase (key_type const& key)
    {
        lock_guard lock (m_mutex);
        if (m_map.erase (key) > 0)
        {
            ++m_stats.hits;
            return true;
        }
        ++m_stats.misses;
        return false;
    }

    
    void sweep ()
    {
        clock_type::time_point const now (m_clock.now ());
        clock_type::time_point when_expire;

        lock_guard lock (m_mutex);

        if (m_target_size == 0 ||
            (m_map.size () <= m_target_size))
        {
            when_expire = now - m_target_age;
        }
        else
        {
            when_expire = now -
                m_target_age * m_target_size / m_map.size();

            clock_type::duration const minimumAge (
                std::chrono::seconds (1));
            if (when_expire > (now - minimumAge))
                when_expire = now - minimumAge;
        }

        iterator it = m_map.begin ();

        while (it != m_map.end ())
        {
            if (it->second.last_access > now)
            {
                it->second.last_access = now;
                ++it;
            }
            else if (it->second.last_access <= when_expire)
            {
                it = m_map.erase (it);
            }
            else
            {
                ++it;
            }
        }
    }

private:
    void collect_metrics ()
    {
        m_stats.size.set (size ());

        {
            beast::insight::Gauge::value_type hit_rate (0);
            {
                lock_guard lock (m_mutex);
                auto const total (m_stats.hits + m_stats.misses);
                if (total != 0)
                    hit_rate = (m_stats.hits * 100) / total;
            }
            m_stats.hit_rate.set (hit_rate);
        }
    }
};

}

#endif








