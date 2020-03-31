

#ifndef RIPPLE_PEERFINDER_BOOTCACHE_H_INCLUDED
#define RIPPLE_PEERFINDER_BOOTCACHE_H_INCLUDED

#include <ripple/peerfinder/PeerfinderManager.h>
#include <ripple/peerfinder/impl/Store.h>
#include <ripple/beast/utility/Journal.h>
#include <ripple/beast/utility/PropertyStream.h>
#include <boost/bimap.hpp>
#include <boost/bimap/multiset_of.hpp>
#include <boost/bimap/unordered_set_of.hpp>
#include <boost/iterator/transform_iterator.hpp>

namespace ripple {
namespace PeerFinder {


class Bootcache
{
private:
    class Entry
    {
    public:
        Entry (int valence)
            : m_valence (valence)
        {
        }

        int& valence ()
        {
            return m_valence;
        }

        int valence () const
        {
            return m_valence;
        }

        friend bool operator< (Entry const& lhs, Entry const& rhs)
        {
            if (lhs.valence() > rhs.valence())
                return true;
            return false;
        }

    private:
        int m_valence;
    };

    using left_t = boost::bimaps::unordered_set_of <beast::IP::Endpoint>;
    using right_t = boost::bimaps::multiset_of <Entry>;
    using map_type = boost::bimap <left_t, right_t>;
    using value_type = map_type::value_type;

    struct Transform
#ifdef _LIBCPP_VERSION
        : std::unary_function<
              map_type::right_map::const_iterator::value_type const&,
              beast::IP::Endpoint const&>
#endif
    {
#ifndef _LIBCPP_VERSION
        using first_argument_type = map_type::right_map::const_iterator::value_type const&;
        using result_type = beast::IP::Endpoint const&;
#endif

        explicit Transform() = default;

        beast::IP::Endpoint const& operator() (
            map_type::right_map::
                const_iterator::value_type const& v) const
        {
            return v.get_left();
        }
    };

private:
    map_type m_map;

    Store& m_store;
    clock_type& m_clock;
    beast::Journal m_journal;

    clock_type::time_point m_whenUpdate;

    bool m_needsUpdate;

public:
    static constexpr int staticValence = 32;

    using iterator = boost::transform_iterator <Transform,
        map_type::right_map::const_iterator>;

    using const_iterator = iterator;

    Bootcache (
        Store& store,
        clock_type& clock,
        beast::Journal journal);

    ~Bootcache ();

    
    bool empty() const;

    
    map_type::size_type size() const;

    
    
    const_iterator begin() const;
    const_iterator cbegin() const;
    const_iterator end() const;
    const_iterator cend() const;
    void clear();
    

    
    void load ();

    
    bool insert (beast::IP::Endpoint const& endpoint);

    
    bool insertStatic (beast::IP::Endpoint const& endpoint);

    
    void on_success (beast::IP::Endpoint const& endpoint);

    
    void on_failure (beast::IP::Endpoint const& endpoint);

    
    void periodicActivity ();

    
    void onWrite (beast::PropertyStream::Map& map);

private:
    void prune ();
    void update ();
    void checkUpdate ();
    void flagForUpdate ();
};

}
}

#endif








