

#ifndef RIPPLE_PEERFINDER_LOGIC_H_INCLUDED
#define RIPPLE_PEERFINDER_LOGIC_H_INCLUDED

#include <ripple/basics/Log.h>
#include <ripple/basics/random.h>
#include <ripple/basics/contract.h>
#include <ripple/beast/container/aged_container_utility.h>
#include <ripple/beast/net/IPAddressConversion.h>
#include <ripple/peerfinder/PeerfinderManager.h>
#include <ripple/peerfinder/impl/Bootcache.h>
#include <ripple/peerfinder/impl/Counts.h>
#include <ripple/peerfinder/impl/Fixed.h>
#include <ripple/peerfinder/impl/Handouts.h>
#include <ripple/peerfinder/impl/Livecache.h>
#include <ripple/peerfinder/impl/Reporting.h>
#include <ripple/peerfinder/impl/SlotImp.h>
#include <ripple/peerfinder/impl/Source.h>
#include <ripple/peerfinder/impl/Store.h>
#include <ripple/peerfinder/impl/iosformat.h>

#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <set>

namespace ripple {
namespace PeerFinder {


template <class Checker>
class Logic
{
public:
    using Slots = std::map<beast::IP::Endpoint,
        std::shared_ptr<SlotImp>>;

    beast::Journal m_journal;
    clock_type& m_clock;
    Store& m_store;
    Checker& m_checker;

    std::recursive_mutex lock_;

    bool stopping_ = false;

    std::shared_ptr<Source> fetchSource_;

    Config config_;

    Counts counts_;

    std::map<beast::IP::Endpoint, Fixed> fixed_;

    Livecache <> livecache_;

    Bootcache bootcache_;

    Slots slots_;

    std::multiset<beast::IP::Address> connectedAddresses_;

    std::set<PublicKey> keys_;

    std::vector<std::shared_ptr<Source>> m_sources;

    clock_type::time_point m_whenBroadcast;

    ConnectHandouts::Squelches m_squelches;


    Logic (clock_type& clock, Store& store,
            Checker& checker, beast::Journal journal)
        : m_journal (journal)
        , m_clock (clock)
        , m_store (store)
        , m_checker (checker)
        , livecache_ (m_clock, journal)
        , bootcache_ (store, m_clock, journal)
        , m_whenBroadcast (m_clock.now())
        , m_squelches (m_clock)
    {
        config ({});
    }

    void load ()
    {
        std::lock_guard<std::recursive_mutex> _(lock_);
        bootcache_.load ();
    }

    
    void stop ()
    {
        std::lock_guard<std::recursive_mutex> _(lock_);
        stopping_ = true;
        if (fetchSource_ != nullptr)
            fetchSource_->cancel ();
    }


    void
    config (Config const& c)
    {
        std::lock_guard<std::recursive_mutex> _(lock_);
        config_ = c;
        counts_.onConfig (config_);
    }

    Config
    config()
    {
        std::lock_guard<std::recursive_mutex> _(lock_);
        return config_;
    }

    void
    addFixedPeer (std::string const& name,
        beast::IP::Endpoint const& ep)
    {
        std::vector<beast::IP::Endpoint> v;
        v.push_back(ep);
        addFixedPeer (name, v);
    }

    void
    addFixedPeer (std::string const& name,
        std::vector <beast::IP::Endpoint> const& addresses)
    {
        std::lock_guard<std::recursive_mutex> _(lock_);

        if (addresses.empty ())
        {
            JLOG(m_journal.info()) <<
                "Could not resolve fixed slot '" << name << "'";
            return;
        }

        for (auto const& remote_address : addresses)
        {
            if (remote_address.port () == 0)
            {
                Throw<std::runtime_error> ("Port not specified for address:" +
                    remote_address.to_string ());
            }

            auto result (fixed_.emplace (std::piecewise_construct,
                std::forward_as_tuple (remote_address),
                    std::make_tuple (std::ref (m_clock))));

            if (result.second)
            {
                JLOG(m_journal.debug()) << beast::leftw (18) <<
                    "Logic add fixed '" << name <<
                    "' at " << remote_address;
                return;
            }
        }
    }


    void checkComplete (beast::IP::Endpoint const& remoteAddress,
        beast::IP::Endpoint const& checkedAddress,
            boost::system::error_code ec)
    {
        if (ec == boost::asio::error::operation_aborted)
            return;

        std::lock_guard<std::recursive_mutex> _(lock_);
        auto const iter (slots_.find (remoteAddress));
        if (iter == slots_.end())
        {
            JLOG(m_journal.debug()) << beast::leftw (18) <<
                "Logic tested " << checkedAddress <<
                " but the connection was closed";
            return;
        }

        SlotImp& slot (*iter->second);
        slot.checked = true;
        slot.connectivityCheckInProgress = false;

        if (ec)
        {
            slot.canAccept = false;
            JLOG(m_journal.error()) << beast::leftw (18) <<
                "Logic testing " << iter->first << " with error, " <<
                ec.message();
            bootcache_.on_failure (checkedAddress);
            return;
        }

        slot.canAccept = true;
        slot.set_listening_port (checkedAddress.port ());
        JLOG(m_journal.debug()) << beast::leftw (18) <<
            "Logic testing " << checkedAddress << " succeeded";
    }


    SlotImp::ptr new_inbound_slot (beast::IP::Endpoint const& local_endpoint,
        beast::IP::Endpoint const& remote_endpoint)
    {
        JLOG(m_journal.debug()) << beast::leftw (18) <<
            "Logic accept" << remote_endpoint <<
            " on local " << local_endpoint;

        std::lock_guard<std::recursive_mutex> _(lock_);

        if (is_public (remote_endpoint))
        {
            auto const count = connectedAddresses_.count (
                remote_endpoint.address());
            if (count > config_.ipLimit)
            {
                JLOG(m_journal.debug()) << beast::leftw (18) <<
                    "Logic dropping inbound " << remote_endpoint <<
                    " because of ip limits.";
                return SlotImp::ptr();
            }
        }

        SlotImp::ptr const slot (std::make_shared <SlotImp> (local_endpoint,
            remote_endpoint, fixed (remote_endpoint.address ()),
                m_clock));
        auto const result (slots_.emplace (
            slot->remote_endpoint (), slot));
        assert (result.second);
        connectedAddresses_.emplace (remote_endpoint.address());

        counts_.add (*slot);

        return result.first->second;
    }

    SlotImp::ptr
    new_outbound_slot (beast::IP::Endpoint const& remote_endpoint)
    {
        JLOG(m_journal.debug()) << beast::leftw (18) <<
            "Logic connect " << remote_endpoint;

        std::lock_guard<std::recursive_mutex> _(lock_);

        if (slots_.find (remote_endpoint) !=
            slots_.end ())
        {
            JLOG(m_journal.debug()) << beast::leftw (18) <<
                "Logic dropping " << remote_endpoint <<
                " as duplicate connect";
            return SlotImp::ptr ();
        }

        SlotImp::ptr const slot (std::make_shared <SlotImp> (
            remote_endpoint, fixed (remote_endpoint), m_clock));

        auto const result =
            slots_.emplace (slot->remote_endpoint (),
                slot);
        assert (result.second);

        connectedAddresses_.emplace (remote_endpoint.address());

        counts_.add (*slot);

        return result.first->second;
    }

    bool
    onConnected (SlotImp::ptr const& slot,
        beast::IP::Endpoint const& local_endpoint)
    {
        JLOG(m_journal.trace()) << beast::leftw (18) <<
            "Logic connected" << slot->remote_endpoint () <<
            " on local " << local_endpoint;

        std::lock_guard<std::recursive_mutex> _(lock_);

        assert (slots_.find (slot->remote_endpoint ()) !=
            slots_.end ());
        slot->local_endpoint (local_endpoint);

        {
            auto const iter (slots_.find (local_endpoint));
            if (iter != slots_.end ())
            {
                assert (iter->second->local_endpoint ()
                        == slot->remote_endpoint ());
                JLOG(m_journal.warn()) << beast::leftw (18) <<
                    "Logic dropping " << slot->remote_endpoint () <<
                    " as self connect";
                return false;
            }
        }

        counts_.remove (*slot);
        slot->state (Slot::connected);
        counts_.add (*slot);
        return true;
    }

    Result
    activate (SlotImp::ptr const& slot,
        PublicKey const& key, bool cluster)
    {
        JLOG(m_journal.debug()) << beast::leftw (18) <<
            "Logic handshake " << slot->remote_endpoint () <<
            " with " << (cluster ? "clustered " : "") << "key " << key;

        std::lock_guard<std::recursive_mutex> _(lock_);

        assert (slots_.find (slot->remote_endpoint ()) !=
            slots_.end ());
        assert (slot->state() == Slot::accept ||
            slot->state() == Slot::connected);

        if (keys_.find (key) != keys_.end())
            return Result::duplicate;

        counts_.remove (*slot);
        slot->cluster (cluster);
        counts_.add (*slot);

        if (! counts_.can_activate (*slot))
        {
            if (! slot->inbound())
                bootcache_.on_success (
                    slot->remote_endpoint());
            return Result::full;
        }

        slot->public_key (key);
        {
            auto const result = keys_.insert(key);
            assert (result.second);
            (void)result.second;
        }

        counts_.remove (*slot);
        slot->activate (m_clock.now ());
        counts_.add (*slot);

        if (! slot->inbound())
            bootcache_.on_success (
                slot->remote_endpoint());

        if (slot->fixed() && ! slot->inbound())
        {
            auto iter (fixed_.find (slot->remote_endpoint()));
            assert (iter != fixed_.end ());
            iter->second.success (m_clock.now ());
            JLOG(m_journal.trace()) << beast::leftw (18) <<
                "Logic fixed " << slot->remote_endpoint () << " success";
        }

        return Result::success;
    }

    
    std::vector <Endpoint>
    redirect (SlotImp::ptr const& slot)
    {
        std::lock_guard<std::recursive_mutex> _(lock_);
        RedirectHandouts h (slot);
        livecache_.hops.shuffle();
        handout (&h, (&h)+1,
            livecache_.hops.begin(),
                livecache_.hops.end());
        return std::move(h.list());
    }

    
    std::vector <beast::IP::Endpoint>
    autoconnect()
    {
        std::vector <beast::IP::Endpoint> const none;

        std::lock_guard<std::recursive_mutex> _(lock_);

        auto needed (counts_.attempts_needed ());
        if (needed == 0)
            return none;

        ConnectHandouts h (needed, m_squelches);

        for (auto const& s : slots_)
        {
            auto const result (m_squelches.insert (
                s.second->remote_endpoint().address()));
            if (! result.second)
                m_squelches.touch (result.first);
        }

        if (counts_.fixed_active() < fixed_.size ())
        {
            get_fixed (needed, h.list(), m_squelches);

            if (! h.list().empty ())
            {
                JLOG(m_journal.debug()) << beast::leftw (18) <<
                    "Logic connect " << h.list().size() << " fixed";
                return h.list();
            }

            if (counts_.attempts() > 0)
            {
                JLOG(m_journal.debug()) << beast::leftw (18) <<
                    "Logic waiting on " <<
                        counts_.attempts() << " attempts";
                return none;
            }
        }

        if (! config_.autoConnect ||
            counts_.out_active () >= counts_.out_max ())
            return none;

        {
            livecache_.hops.shuffle();
            handout (&h, (&h)+1,
                livecache_.hops.rbegin(),
                    livecache_.hops.rend());
            if (! h.list().empty ())
            {
                JLOG(m_journal.debug()) << beast::leftw (18) <<
                    "Logic connect " << h.list().size () << " live " <<
                    ((h.list().size () > 1) ? "endpoints" : "endpoint");
                return h.list();
            }
            else if (counts_.attempts() > 0)
            {
                JLOG(m_journal.debug()) << beast::leftw (18) <<
                    "Logic waiting on " <<
                        counts_.attempts() << " attempts";
                return none;
            }
        }

        

        for (auto iter (bootcache_.begin());
            ! h.full() && iter != bootcache_.end(); ++iter)
            h.try_insert (*iter);

        if (! h.list().empty ())
        {
            JLOG(m_journal.debug()) << beast::leftw (18) <<
                "Logic connect " << h.list().size () << " boot " <<
                ((h.list().size () > 1) ? "addresses" : "address");
            return h.list();
        }

        return none;
    }

    std::vector<std::pair<Slot::ptr, std::vector<Endpoint>>>
    buildEndpointsForPeers()
    {
        std::vector<std::pair<Slot::ptr, std::vector<Endpoint>>> result;

        std::lock_guard<std::recursive_mutex> _(lock_);

        clock_type::time_point const now = m_clock.now();
        if (m_whenBroadcast <= now)
        {
            std::vector <SlotHandouts> targets;

            {
                std::vector <SlotImp::ptr> slots;
                slots.reserve (slots_.size());
                std::for_each (slots_.cbegin(), slots_.cend(),
                    [&slots](Slots::value_type const& value)
                    {
                        if (value.second->state() == Slot::active)
                            slots.emplace_back (value.second);
                    });
                std::shuffle (slots.begin(), slots.end(), default_prng());

                targets.reserve (slots.size());
                std::for_each (slots.cbegin(), slots.cend(),
                    [&targets](SlotImp::ptr const& slot)
                    {
                        targets.emplace_back (slot);
                    });
            }

            
            if (config_.wantIncoming &&
                counts_.inboundSlots() > 0)
            {
                Endpoint ep;
                ep.hops = 0;
                ep.address = beast::IP::Endpoint (
                    beast::IP::AddressV6 ()).at_port (
                        config_.listeningPort);
                for (auto& t : targets)
                    t.insert (ep);
            }

            livecache_.hops.shuffle();
            handout (targets.begin(), targets.end(),
                livecache_.hops.begin(),
                    livecache_.hops.end());

            for (auto const& t : targets)
            {
                SlotImp::ptr const& slot = t.slot();
                auto const& list = t.list();
                JLOG(m_journal.trace()) << beast::leftw (18) <<
                    "Logic sending " << slot->remote_endpoint() <<
                    " with " << list.size() <<
                    ((list.size() == 1) ? " endpoint" : " endpoints");
                result.push_back (std::make_pair (slot, list));
            }

            m_whenBroadcast = now + Tuning::secondsPerMessage;
        }

        return result;
    }

    void once_per_second()
    {
        std::lock_guard<std::recursive_mutex> _(lock_);

        livecache_.expire ();

        for (auto const& entry : slots_)
            entry.second->expire();

        beast::expire (m_squelches,
            Tuning::recentAttemptDuration);

        bootcache_.periodicActivity ();
    }


    void preprocess (SlotImp::ptr const& slot, Endpoints& list)
    {
        bool neighbor (false);
        for (auto iter = list.begin(); iter != list.end();)
        {
            Endpoint& ep (*iter);

            if (ep.hops > Tuning::maxHops)
            {
                JLOG(m_journal.debug()) << beast::leftw (18) <<
                    "Endpoints drop " << ep.address <<
                    " for excess hops " << ep.hops;
                iter = list.erase (iter);
                continue;
            }

            if (ep.hops == 0)
            {
                if (! neighbor)
                {
                    neighbor = true;
                    ep.address = slot->remote_endpoint().at_port (
                        ep.address.port ());
                }
                else
                {
                    JLOG(m_journal.debug()) << beast::leftw (18) <<
                        "Endpoints drop " << ep.address <<
                        " for extra self";
                    iter = list.erase (iter);
                    continue;
                }
            }

            if (! is_valid_address (ep.address))
            {
                JLOG(m_journal.debug()) << beast::leftw (18) <<
                    "Endpoints drop " << ep.address <<
                    " as invalid";
                iter = list.erase (iter);
                continue;
            }

            if (std::any_of (list.begin(), iter,
                [ep](Endpoints::value_type const& other)
                {
                    return ep.address == other.address;
                }))
            {
                JLOG(m_journal.debug()) << beast::leftw (18) <<
                    "Endpoints drop " << ep.address <<
                    " as duplicate";
                iter = list.erase (iter);
                continue;
            }

            ++ep.hops;

            ++iter;
        }
    }

    void on_endpoints (SlotImp::ptr const& slot, Endpoints list)
    {
        JLOG(m_journal.trace()) << beast::leftw (18) <<
            "Endpoints from " << slot->remote_endpoint () <<
            " contained " << list.size () <<
            ((list.size() > 1) ? " entries" : " entry");

        std::lock_guard<std::recursive_mutex> _(lock_);

        assert (slots_.find (slot->remote_endpoint ()) !=
            slots_.end ());

        assert (slot->state() == Slot::active);

        preprocess (slot, list);

        clock_type::time_point const now (m_clock.now());

        for (auto const& ep : list)
        {
            assert (ep.hops != 0);

            slot->recent.insert (ep.address, ep.hops);

            if (ep.hops == 1)
            {
                if (slot->connectivityCheckInProgress)
                {
                    JLOG(m_journal.debug()) << beast::leftw (18) <<
                        "Logic testing " << ep.address << " already in progress";
                    continue;
                }

                if (! slot->checked)
                {
                    slot->connectivityCheckInProgress = true;

                    m_checker.async_connect (ep.address, std::bind (
                        &Logic::checkComplete, this, slot->remote_endpoint(),
                            ep.address, std::placeholders::_1));


                    continue;
                }

                if (! slot->canAccept)
                    continue;
            }

            livecache_.insert (ep);
            bootcache_.insert (ep.address);
        }

        slot->whenAcceptEndpoints = now + Tuning::secondsPerMessage;
    }


    void remove (SlotImp::ptr const& slot)
    {
        auto const iter = slots_.find (
            slot->remote_endpoint ());
        assert (iter != slots_.end ());
        slots_.erase (iter);
        if (slot->public_key () != boost::none)
        {
            auto const iter = keys_.find (*slot->public_key());
            assert (iter != keys_.end ());
            keys_.erase (iter);
        }
        {
            auto const iter (connectedAddresses_.find (
                slot->remote_endpoint().address()));
            assert (iter != connectedAddresses_.end ());
            connectedAddresses_.erase (iter);
        }

        counts_.remove (*slot);
    }

    void on_closed (SlotImp::ptr const& slot)
    {
        std::lock_guard<std::recursive_mutex> _(lock_);

        remove (slot);

        if (slot->fixed() && ! slot->inbound() && slot->state() != Slot::active)
        {
            auto iter (fixed_.find (slot->remote_endpoint()));
            assert (iter != fixed_.end ());
            iter->second.failure (m_clock.now ());
            JLOG(m_journal.debug()) << beast::leftw (18) <<
                "Logic fixed " << slot->remote_endpoint () << " failed";
        }

        switch (slot->state())
        {
        case Slot::accept:
            JLOG(m_journal.trace()) << beast::leftw (18) <<
                "Logic accept " << slot->remote_endpoint () << " failed";
            break;

        case Slot::connect:
        case Slot::connected:
            bootcache_.on_failure (slot->remote_endpoint ());
            break;

        case Slot::active:
            JLOG(m_journal.trace()) << beast::leftw (18) <<
                "Logic close " << slot->remote_endpoint();
            break;

        case Slot::closing:
            JLOG(m_journal.trace()) << beast::leftw (18) <<
                "Logic finished " << slot->remote_endpoint();
            break;

        default:
            assert (false);
            break;
        }
    }

    void on_failure (SlotImp::ptr const& slot)
    {
        std::lock_guard<std::recursive_mutex> _(lock_);

        bootcache_.on_failure (slot->remote_endpoint ());
    }

    template <class FwdIter>
    void
    onRedirects (FwdIter first, FwdIter last,
        boost::asio::ip::tcp::endpoint const& remote_address);


    bool fixed (beast::IP::Endpoint const& endpoint) const
    {
        for (auto const& entry : fixed_)
            if (entry.first == endpoint)
                return true;
        return false;
    }

    bool fixed (beast::IP::Address const& address) const
    {
        for (auto const& entry : fixed_)
            if (entry.first.address () == address)
                return true;
        return false;
    }


    
    template <class Container>
    void get_fixed (std::size_t needed, Container& c,
        typename ConnectHandouts::Squelches& squelches)
    {
        auto const now (m_clock.now());
        for (auto iter = fixed_.begin ();
            needed && iter != fixed_.end (); ++iter)
        {
            auto const& address (iter->first.address());
            if (iter->second.when() <= now && squelches.find(address) ==
                    squelches.end() && std::none_of (
                        slots_.cbegin(), slots_.cend(),
                    [address](Slots::value_type const& v)
                    {
                        return address == v.first.address();
                    }))
            {
                squelches.insert(iter->first.address());
                c.push_back (iter->first);
                --needed;
            }
        }
    }


    void
    addStaticSource (std::shared_ptr<Source> const& source)
    {
        fetch (source);
    }

    void
    addSource (std::shared_ptr<Source> const& source)
    {
        m_sources.push_back (source);
    }


    int addBootcacheAddresses (IPAddresses const& list)
    {
        int count (0);
        std::lock_guard<std::recursive_mutex> _(lock_);
        for (auto addr : list)
        {
            if (bootcache_.insertStatic (addr))
                ++count;
        }
        return count;
    }

    void
    fetch (std::shared_ptr<Source> const& source)
    {
        Source::Results results;

        {
            {
                std::lock_guard<std::recursive_mutex> _(lock_);
                if (stopping_)
                    return;
                fetchSource_ = source;
            }

            source->fetch (results, m_journal);

            {
                std::lock_guard<std::recursive_mutex> _(lock_);
                if (stopping_)
                    return;
                fetchSource_ = nullptr;
            }
        }

        if (! results.error)
        {
            int const count (addBootcacheAddresses (results.addresses));
            JLOG(m_journal.info()) << beast::leftw (18) <<
                "Logic added " << count <<
                " new " << ((count == 1) ? "address" : "addresses") <<
                " from " << source->name();
        }
        else
        {
            JLOG(m_journal.error()) << beast::leftw (18) <<
                "Logic failed " << "'" << source->name() << "' fetch, " <<
                results.error.message();
        }
    }


    bool is_valid_address (beast::IP::Endpoint const& address)
    {
        if (is_unspecified (address))
            return false;
        if (! is_public (address))
            return false;
        if (address.port() == 0)
            return false;
        return true;
    }


    void writeSlots (beast::PropertyStream::Set& set, Slots const& slots)
    {
        for (auto const& entry : slots)
        {
            beast::PropertyStream::Map item (set);
            SlotImp const& slot (*entry.second);
            if (slot.local_endpoint () != boost::none)
                item ["local_address"] = to_string (*slot.local_endpoint ());
            item ["remote_address"]   = to_string (slot.remote_endpoint ());
            if (slot.inbound())
                item ["inbound"]    = "yes";
            if (slot.fixed())
                item ["fixed"]      = "yes";
            if (slot.cluster())
                item ["cluster"]    = "yes";

            item ["state"] = stateString (slot.state());
        }
    }

    void onWrite (beast::PropertyStream::Map& map)
    {
        std::lock_guard<std::recursive_mutex> _(lock_);

        map ["bootcache"]   = std::uint32_t (bootcache_.size());
        map ["fixed"]       = std::uint32_t (fixed_.size());

        {
            beast::PropertyStream::Set child ("peers", map);
            writeSlots (child, slots_);
        }

        {
            beast::PropertyStream::Map child ("counts", map);
            counts_.onWrite (child);
        }

        {
            beast::PropertyStream::Map child ("config", map);
            config_.onWrite (child);
        }

        {
            beast::PropertyStream::Map child ("livecache", map);
            livecache_.onWrite (child);
        }

        {
            beast::PropertyStream::Map child ("bootcache", map);
            bootcache_.onWrite (child);
        }
    }


    Counts const& counts () const
    {
        return counts_;
    }

    static std::string stateString (Slot::State state)
    {
        switch (state)
        {
        case Slot::accept:     return "accept";
        case Slot::connect:    return "connect";
        case Slot::connected:  return "connected";
        case Slot::active:     return "active";
        case Slot::closing:    return "closing";
        default:
            break;
        };
        return "?";
    }
};


template <class Checker>
template <class FwdIter>
void
Logic<Checker>::onRedirects (FwdIter first, FwdIter last,
    boost::asio::ip::tcp::endpoint const& remote_address)
{
    std::lock_guard<std::recursive_mutex> _(lock_);
    std::size_t n = 0;
    for(;first != last && n < Tuning::maxRedirects; ++first, ++n)
        bootcache_.insert(
            beast::IPAddressConversion::from_asio(*first));
    if (n > 0)
    {
        JLOG(m_journal.trace()) << beast::leftw (18) <<
            "Logic add " << n << " redirect IPs from " << remote_address;
    }
}

}
}

#endif








