

#ifndef RIPPLE_SHAMAP_FULLBELOWCACHE_H_INCLUDED
#define RIPPLE_SHAMAP_FULLBELOWCACHE_H_INCLUDED

#include <ripple/basics/base_uint.h>
#include <ripple/basics/KeyCache.h>
#include <ripple/beast/insight/Collector.h>
#include <atomic>
#include <string>

namespace ripple {

namespace detail {


template <class Key>
class BasicFullBelowCache
{
private:
    using CacheType = KeyCache <Key>;

public:
    enum
    {
         defaultCacheTargetSize = 0
    };

    using key_type   = Key;
    using size_type  = typename CacheType::size_type;
    using clock_type = typename CacheType::clock_type;

    
    BasicFullBelowCache (std::string const& name, clock_type& clock,
        beast::insight::Collector::ptr const& collector =
            beast::insight::NullCollector::New (),
        std::size_t target_size = defaultCacheTargetSize,
        std::chrono::seconds expiration = std::chrono::minutes{2})
        : m_cache (name, clock, collector, target_size,
            expiration)
        , m_gen (1)
    {
    }

    
    clock_type& clock()
    {
        return m_cache.clock ();
    }

    
    size_type size () const
    {
        return m_cache.size ();
    }

    
    void sweep ()
    {
        m_cache.sweep ();
    }

    
    bool touch_if_exists (key_type const& key)
    {
        return m_cache.touch_if_exists (key);
    }

    
    void insert (key_type const& key)
    {
        m_cache.insert (key);
    }

    
    std::uint32_t getGeneration (void) const
    {
        return m_gen;
    }

    void clear ()
    {
        m_cache.clear ();
        ++m_gen;
    }

    void reset ()
    {
        m_cache.clear();
        m_gen  = 1;
    }

private:
    KeyCache <Key> m_cache;
    std::atomic <std::uint32_t> m_gen;
};

} 

using FullBelowCache = detail::BasicFullBelowCache <uint256>;

}

#endif








