

#ifndef RIPPLE_PEERFINDER_LIVECACHE_H_INCLUDED
#define RIPPLE_PEERFINDER_LIVECACHE_H_INCLUDED

#include <ripple/basics/Log.h>
#include <ripple/basics/random.h>
#include <ripple/peerfinder/PeerfinderManager.h>
#include <ripple/peerfinder/impl/iosformat.h>
#include <ripple/peerfinder/impl/Tuning.h>
#include <ripple/beast/container/aged_map.h>
#include <ripple/beast/utility/maybe_const.h>
#include <boost/intrusive/list.hpp>
#include <boost/iterator/transform_iterator.hpp>

#include <algorithm>

namespace ripple {
namespace PeerFinder {

template <class>
class Livecache;

namespace detail {

class LivecacheBase
{
public:
    explicit LivecacheBase() = default;
protected:
    struct Element
        : boost::intrusive::list_base_hook <>
    {
        Element (Endpoint const& endpoint_)
            : endpoint (endpoint_)
        {
        }

        Endpoint endpoint;
    };

    using list_type = boost::intrusive::make_list <Element,
        boost::intrusive::constant_time_size <false>
            >::type;

public:
    
    template <bool IsConst>
    class Hop
    {
    public:
        struct Transform
#ifdef _LIBCPP_VERSION
            : public std::unary_function<Element, Endpoint>
#endif
        {
#ifndef _LIBCPP_VERSION
            using first_argument = Element;
            using result_type = Endpoint;
#endif

            explicit Transform() = default;

            Endpoint const& operator() (Element const& e) const
            {
                return e.endpoint;
            }
        };

    public:
        using iterator = boost::transform_iterator <Transform,
            typename list_type::const_iterator>;

        using const_iterator = iterator;

        using reverse_iterator = boost::transform_iterator <Transform,
            typename list_type::const_reverse_iterator>;

        using const_reverse_iterator = reverse_iterator;

        iterator begin () const
        {
            return iterator (m_list.get().cbegin(),
                Transform());
        }

        iterator cbegin () const
        {
            return iterator (m_list.get().cbegin(),
                Transform());
        }

        iterator end () const
        {
            return iterator (m_list.get().cend(),
                Transform());
        }

        iterator cend () const
        {
            return iterator (m_list.get().cend(),
                Transform());
        }

        reverse_iterator rbegin () const
        {
            return reverse_iterator (m_list.get().crbegin(),
                Transform());
        }

        reverse_iterator crbegin () const
        {
            return reverse_iterator (m_list.get().crbegin(),
                Transform());
        }

        reverse_iterator rend () const
        {
            return reverse_iterator (m_list.get().crend(),
                Transform());
        }

        reverse_iterator crend () const
        {
            return reverse_iterator (m_list.get().crend(),
                Transform());
        }

        void move_back (const_iterator pos)
        {
            auto& e (const_cast <Element&>(*pos.base()));
            m_list.get().erase (m_list.get().iterator_to (e));
            m_list.get().push_back (e);
        }

    private:
        explicit Hop (typename beast::maybe_const <
            IsConst, list_type>::type& list)
            : m_list (list)
        {
        }

        friend class LivecacheBase;

        std::reference_wrapper <typename beast::maybe_const <
            IsConst, list_type>::type> m_list;
    };

protected:
    template <bool IsConst>
    static Hop <IsConst> make_hop (typename beast::maybe_const <
        IsConst, list_type>::type& list)
    {
        return Hop <IsConst> (list);
    }
};

}



template <class Allocator = std::allocator <char>>
class Livecache : protected detail::LivecacheBase
{
private:
    using cache_type = beast::aged_map <beast::IP::Endpoint, Element,
        std::chrono::steady_clock, std::less <beast::IP::Endpoint>,
            Allocator>;

    beast::Journal m_journal;
    cache_type m_cache;

public:
    using allocator_type = Allocator;

    
    Livecache (
        clock_type& clock,
        beast::Journal journal,
        Allocator alloc = Allocator());


    class hops_t
    {
    private:
        using Histogram = std::array <int, 1 + Tuning::maxHops + 1>;
        using lists_type = std::array <list_type, 1 + Tuning::maxHops + 1>;

        template <bool IsConst>
        struct Transform
#ifdef _LIBCPP_VERSION
            : public std::unary_function<typename lists_type::value_type, Hop<IsConst>>
#endif
        {
#ifndef _LIBCPP_VERSION
            using first_argument = typename lists_type::value_type;
            using result_type = Hop <IsConst>;
#endif

            explicit Transform() = default;

            Hop <IsConst> operator() (typename beast::maybe_const <
                IsConst, typename lists_type::value_type>::type& list) const
            {
                return make_hop <IsConst> (list);
            }
        };

    public:
        using iterator = boost::transform_iterator <Transform <false>,
            typename lists_type::iterator>;

        using const_iterator = boost::transform_iterator <Transform <true>,
            typename lists_type::const_iterator>;

        using reverse_iterator = boost::transform_iterator <Transform <false>,
            typename lists_type::reverse_iterator>;

        using const_reverse_iterator = boost::transform_iterator <Transform <true>,
            typename lists_type::const_reverse_iterator>;

        iterator begin ()
        {
            return iterator (m_lists.begin(),
                Transform <false>());
        }

        const_iterator begin () const
        {
            return const_iterator (m_lists.cbegin(),
                Transform <true>());
        }

        const_iterator cbegin () const
        {
            return const_iterator (m_lists.cbegin(),
                Transform <true>());
        }

        iterator end ()
        {
            return iterator (m_lists.end(),
                Transform <false>());
        }

        const_iterator end () const
        {
            return const_iterator (m_lists.cend(),
                Transform <true>());
        }

        const_iterator cend () const
        {
            return const_iterator (m_lists.cend(),
                Transform <true>());
        }

        reverse_iterator rbegin ()
        {
            return reverse_iterator (m_lists.rbegin(),
                Transform <false>());
        }

        const_reverse_iterator rbegin () const
        {
            return const_reverse_iterator (m_lists.crbegin(),
                Transform <true>());
        }

        const_reverse_iterator crbegin () const
        {
            return const_reverse_iterator (m_lists.crbegin(),
                Transform <true>());
        }

        reverse_iterator rend ()
        {
            return reverse_iterator (m_lists.rend(),
                Transform <false>());
        }

        const_reverse_iterator rend () const
        {
            return const_reverse_iterator (m_lists.crend(),
                Transform <true>());
        }

        const_reverse_iterator crend () const
        {
            return const_reverse_iterator (m_lists.crend(),
                Transform <true>());
        }

        
        void shuffle ();

        std::string histogram() const;

    private:
        explicit hops_t (Allocator const& alloc);

        void insert (Element& e);

        void reinsert (Element& e, int hops);

        void remove (Element& e);

        friend class Livecache;
        lists_type m_lists;
        Histogram m_hist;
    } hops;

    
    bool empty () const
    {
        return m_cache.empty ();
    }

    
    typename cache_type::size_type size() const
    {
        return m_cache.size();
    }

    
    void expire ();

    
    void insert (Endpoint const& ep);

    
    void onWrite (beast::PropertyStream::Map& map);
};


template <class Allocator>
Livecache <Allocator>::Livecache (
    clock_type& clock,
    beast::Journal journal,
    Allocator alloc)
    : m_journal (journal)
    , m_cache (clock, alloc)
    , hops (alloc)
{
}

template <class Allocator>
void
Livecache <Allocator>::expire()
{
    std::size_t n (0);
    typename cache_type::time_point const expired (
        m_cache.clock().now() - Tuning::liveCacheSecondsToLive);
    for (auto iter (m_cache.chronological.begin());
        iter != m_cache.chronological.end() && iter.when() <= expired;)
    {
        Element& e (iter->second);
        hops.remove (e);
        iter = m_cache.erase (iter);
        ++n;
    }
    if (n > 0)
    {
        JLOG(m_journal.debug()) << beast::leftw (18) <<
            "Livecache expired " << n <<
            ((n > 1) ? " entries" : " entry");
    }
}

template <class Allocator>
void Livecache <Allocator>::insert (Endpoint const& ep)
{
    assert (ep.hops <= (Tuning::maxHops + 1));
    std::pair <typename cache_type::iterator, bool> result (
        m_cache.emplace (ep.address, ep));
    Element& e (result.first->second);
    if (result.second)
    {
        hops.insert (e);
        JLOG(m_journal.debug()) << beast::leftw (18) <<
            "Livecache insert " << ep.address <<
            " at hops " << ep.hops;
        return;
    }
    else if (! result.second && (ep.hops > e.endpoint.hops))
    {
        std::size_t const excess (
            ep.hops - e.endpoint.hops);
        JLOG(m_journal.trace()) << beast::leftw(18) <<
            "Livecache drop " << ep.address <<
            " at hops +" << excess;
        return;
    }

    m_cache.touch (result.first);

    if (ep.hops < e.endpoint.hops)
    {
        hops.reinsert (e, ep.hops);
        JLOG(m_journal.debug()) << beast::leftw (18) <<
            "Livecache update " << ep.address <<
            " at hops " << ep.hops;
    }
    else
    {
        JLOG(m_journal.trace()) << beast::leftw (18) <<
            "Livecache refresh " << ep.address <<
            " at hops " << ep.hops;
    }
}

template <class Allocator>
void
Livecache <Allocator>::onWrite (beast::PropertyStream::Map& map)
{
    typename cache_type::time_point const expired (
        m_cache.clock().now() - Tuning::liveCacheSecondsToLive);
    map ["size"] = size ();
    map ["hist"] = hops.histogram();
    beast::PropertyStream::Set set ("entries", map);
    for (auto iter (m_cache.cbegin()); iter != m_cache.cend(); ++iter)
    {
        auto const& e (iter->second);
        beast::PropertyStream::Map item (set);
        item ["hops"] = e.endpoint.hops;
        item ["address"] = e.endpoint.address.to_string ();
        std::stringstream ss;
        ss << (iter.when() - expired).count();
        item ["expires"] = ss.str();
    }
}


template <class Allocator>
void
Livecache <Allocator>::hops_t::shuffle()
{
    for (auto& list : m_lists)
    {
        std::vector <std::reference_wrapper <Element>> v;
        v.reserve (list.size());
        std::copy (list.begin(), list.end(),
            std::back_inserter (v));
        std::shuffle (v.begin(), v.end(), default_prng());
        list.clear();
        for (auto& e : v)
            list.push_back (e);
    }
}

template <class Allocator>
std::string
Livecache <Allocator>::hops_t::histogram() const
{
    std::stringstream ss;
    for (typename decltype(m_hist)::size_type i (0);
        i < m_hist.size(); ++i)
    {
        ss << m_hist[i] <<
            ((i < Tuning::maxHops + 1) ? ", " : "");
    }
    return ss.str();
}

template <class Allocator>
Livecache <Allocator>::hops_t::hops_t (Allocator const& alloc)
{
    std::fill (m_hist.begin(), m_hist.end(), 0);
}

template <class Allocator>
void
Livecache <Allocator>::hops_t::insert (Element& e)
{
    assert (e.endpoint.hops >= 0 &&
        e.endpoint.hops <= Tuning::maxHops + 1);
    m_lists [e.endpoint.hops].push_front (e);
    ++m_hist [e.endpoint.hops];
}

template <class Allocator>
void
Livecache <Allocator>::hops_t::reinsert (Element& e, int hops)
{
    assert (hops >= 0 && hops <= Tuning::maxHops + 1);
    list_type& list (m_lists [e.endpoint.hops]);
    list.erase (list.iterator_to (e));
    --m_hist [e.endpoint.hops];

    e.endpoint.hops = hops;
    insert (e);
}

template <class Allocator>
void
Livecache <Allocator>::hops_t::remove (Element& e)
{
    --m_hist [e.endpoint.hops];
    list_type& list (m_lists [e.endpoint.hops]);
    list.erase (list.iterator_to (e));
}

}
}

#endif








