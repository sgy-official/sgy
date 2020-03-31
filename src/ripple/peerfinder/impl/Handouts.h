

#ifndef RIPPLE_PEERFINDER_HANDOUTS_H_INCLUDED
#define RIPPLE_PEERFINDER_HANDOUTS_H_INCLUDED

#include <ripple/peerfinder/impl/SlotImp.h>
#include <ripple/peerfinder/impl/Tuning.h>
#include <ripple/beast/container/aged_set.h>
#include <cassert>
#include <iterator>
#include <type_traits>

namespace ripple {
namespace PeerFinder {

namespace detail {


template <class Target, class HopContainer>
std::size_t
handout_one (Target& t, HopContainer& h)
{
    assert (! t.full());
    for (auto it = h.begin(); it != h.end(); ++it)
    {
        auto const& e = *it;
        if (t.try_insert (e))
        {
            h.move_back (it);
            return 1;
        }
    }
    return 0;
}

}


template <class TargetFwdIter, class SeqFwdIter>
void
handout (TargetFwdIter first, TargetFwdIter last,
    SeqFwdIter seq_first, SeqFwdIter seq_last)
{
    for (;;)
    {
        std::size_t n (0);
        for (auto si = seq_first; si != seq_last; ++si)
        {
            auto c = *si;
            bool all_full (true);
            for (auto ti = first; ti != last; ++ti)
            {
                auto& t = *ti;
                if (! t.full())
                {
                    n += detail::handout_one (t, c);
                    all_full = false;
                }
            }
            if (all_full)
                return;
        }
        if (! n)
            break;
    }
}



class RedirectHandouts
{
public:
    template <class = void>
    explicit
    RedirectHandouts (SlotImp::ptr const& slot);

    template <class = void>
    bool try_insert (Endpoint const& ep);

    bool full () const
    {
        return list_.size() >= Tuning::redirectEndpointCount;
    }

    SlotImp::ptr const& slot () const
    {
        return slot_;
    }

    std::vector <Endpoint>& list()
    {
        return list_;
    }

    std::vector <Endpoint> const& list() const
    {
        return list_;
    }

private:
    SlotImp::ptr slot_;
    std::vector <Endpoint> list_;
};

template <class>
RedirectHandouts::RedirectHandouts (SlotImp::ptr const& slot)
    : slot_ (slot)
{
    list_.reserve (Tuning::redirectEndpointCount);
}

template <class>
bool
RedirectHandouts::try_insert (Endpoint const& ep)
{
    if (full ())
        return false;

    if (ep.hops > Tuning::maxHops)
        return false;

    if (ep.hops == 0)
        return false;

    if (slot_->remote_endpoint().address() ==
        ep.address.address())
        return false;

    if (std::any_of (list_.begin(), list_.end(),
        [&ep](Endpoint const& other)
        {
            return other.address.address() == ep.address.address();
        }))
    {
        return false;
    }

    list_.emplace_back (ep.address, ep.hops);

    return true;
}



class SlotHandouts
{
public:
    template <class = void>
    explicit
    SlotHandouts (SlotImp::ptr const& slot);

    template <class = void>
    bool try_insert (Endpoint const& ep);

    bool full () const
    {
        return list_.size() >= Tuning::numberOfEndpoints;
    }

    void insert (Endpoint const& ep)
    {
        list_.push_back (ep);
    }

    SlotImp::ptr const& slot () const
    {
        return slot_;
    }

    std::vector <Endpoint> const& list() const
    {
        return list_;
    }

private:
    SlotImp::ptr slot_;
    std::vector <Endpoint> list_;
};

template <class>
SlotHandouts::SlotHandouts (SlotImp::ptr const& slot)
    : slot_ (slot)
{
    list_.reserve (Tuning::numberOfEndpoints);
}

template <class>
bool
SlotHandouts::try_insert (Endpoint const& ep)
{
    if (full ())
        return false;

    if (ep.hops > Tuning::maxHops)
        return false;

    if (slot_->recent.filter (ep.address, ep.hops))
        return false;

    if (slot_->remote_endpoint().address() ==
        ep.address.address())
        return false;

    if (std::any_of (list_.begin(), list_.end(),
        [&ep](Endpoint const& other)
        {
            return other.address.address() == ep.address.address();
        }))
        return false;

    list_.emplace_back (ep.address, ep.hops);

    slot_->recent.insert (ep.address, ep.hops);

    return true;
}



class ConnectHandouts
{
public:
    using Squelches = beast::aged_set <beast::IP::Address>;

    using list_type = std::vector <beast::IP::Endpoint>;

private:
    std::size_t m_needed;
    Squelches& m_squelches;
    list_type m_list;

public:
    template <class = void>
    ConnectHandouts (std::size_t needed, Squelches& squelches);

    template <class = void>
    bool try_insert (beast::IP::Endpoint const& endpoint);

    bool empty() const
    {
        return m_list.empty();
    }

    bool full() const
    {
        return m_list.size() >= m_needed;
    }

    bool try_insert (Endpoint const& endpoint)
    {
        return try_insert (endpoint.address);
    }

    list_type& list()
    {
        return m_list;
    }

    list_type const& list() const
    {
        return m_list;
    }
};

template <class>
ConnectHandouts::ConnectHandouts (
    std::size_t needed, Squelches& squelches)
    : m_needed (needed)
    , m_squelches (squelches)
{
    m_list.reserve (needed);
}

template <class>
bool
ConnectHandouts::try_insert (beast::IP::Endpoint const& endpoint)
{
    if (full ())
        return false;

    if (std::any_of (m_list.begin(), m_list.end(),
        [&endpoint](beast::IP::Endpoint const& other)
        {
            return other.address() ==
                endpoint.address();
        }))
    {
        return false;
    }

    auto const result (m_squelches.insert (
        endpoint.address()));
    if (! result.second)
        return false;

    m_list.push_back (endpoint);

    return true;
}

}
}

#endif








