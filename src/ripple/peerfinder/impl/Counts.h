

#ifndef RIPPLE_PEERFINDER_COUNTS_H_INCLUDED
#define RIPPLE_PEERFINDER_COUNTS_H_INCLUDED

#include <ripple/basics/random.h>
#include <ripple/peerfinder/PeerfinderManager.h>
#include <ripple/peerfinder/Slot.h>
#include <ripple/peerfinder/impl/Tuning.h>

namespace ripple {
namespace PeerFinder {


class Counts
{
public:
    Counts ()
        : m_attempts (0)
        , m_active (0)
        , m_in_max (0)
        , m_in_active (0)
        , m_out_max (0)
        , m_out_active (0)
        , m_fixed (0)
        , m_fixed_active (0)
        , m_cluster (0)

        , m_acceptCount (0)
        , m_closingCount (0)
    {
        m_roundingThreshold =
            std::generate_canonical <double, 10> (
                default_prng());
    }


    
    void add (Slot const& s)
    {
        adjust (s, 1);
    }

    
    void remove (Slot const& s)
    {
        adjust (s, -1);
    }

    
    bool can_activate (Slot const& s) const
    {
        assert (s.state() == Slot::connected || s.state() == Slot::accept);

        if (s.fixed () || s.cluster ())
            return true;

        if (s.inbound ())
            return m_in_active < m_in_max;

        return m_out_active < m_out_max;
    }

    
    std::size_t attempts_needed () const
    {
        if (m_attempts >= Tuning::maxConnectAttempts)
            return 0;
        return Tuning::maxConnectAttempts - m_attempts;
    }

    
    std::size_t attempts () const
    {
        return m_attempts;
    }

    
    int out_max () const
    {
        return m_out_max;
    }

    
    int out_active () const
    {
        return m_out_active;
    }

    
    std::size_t fixed () const
    {
        return m_fixed;
    }

    
    std::size_t fixed_active () const
    {
        return m_fixed_active;
    }


    
    void onConfig (Config const& config)
    {
        if (config.wantIncoming)
        {
            m_out_max = std::floor (config.outPeers);
            if (m_roundingThreshold < (config.outPeers - m_out_max))
                ++m_out_max;
        }
        else
        {
            m_out_max = config.maxPeers;
        }

        if (config.maxPeers >= m_out_max)
            m_in_max = config.maxPeers - m_out_max;
        else
            m_in_max = 0;
    }

    
    int acceptCount() const
    {
        return m_acceptCount;
    }

    
    int connectCount() const
    {
        return m_attempts;
    }

    
    int closingCount () const
    {
        return m_closingCount;
    }

    
    int inboundSlots () const
    {
        return m_in_max;
    }

    
    int inboundActive () const
    {
        return m_in_active;
    }

    
    int totalActive () const
    {
        return m_in_active + m_out_active;
    }

    
    int inboundSlotsFree () const
    {
        if (m_in_active < m_in_max)
            return m_in_max - m_in_active;
        return 0;
    }

    
    int outboundSlotsFree () const
    {
        if (m_out_active < m_out_max)
            return m_out_max - m_out_active;
        return 0;
    }


    
    bool isConnectedToNetwork () const
    {

        if (m_out_max > 0)
            return false;

        return true;
    }

    
    void onWrite (beast::PropertyStream::Map& map)
    {
        map ["accept"]  = acceptCount ();
        map ["connect"] = connectCount ();
        map ["close"]   = closingCount ();
        map ["in"]      << m_in_active << "/" << m_in_max;
        map ["out"]     << m_out_active << "/" << m_out_max;
        map ["fixed"]   = m_fixed_active;
        map ["cluster"] = m_cluster;
        map ["total"]   = m_active;
    }

    
    std::string state_string () const
    {
        std::stringstream ss;
        ss <<
            m_out_active << "/" << m_out_max << " out, " <<
            m_in_active << "/" << m_in_max << " in, " <<
            connectCount() << " connecting, " <<
            closingCount() << " closing"
            ;
        return ss.str();
    }

private:
    void adjust (Slot const& s, int const n)
    {
        if (s.fixed ())
            m_fixed += n;

        if (s.cluster ())
            m_cluster += n;

        switch (s.state ())
        {
        case Slot::accept:
            assert (s.inbound ());
            m_acceptCount += n;
            break;

        case Slot::connect:
        case Slot::connected:
            assert (! s.inbound ());
            m_attempts += n;
            break;

        case Slot::active:
            if (s.fixed ())
                m_fixed_active += n;
            if (! s.fixed () && ! s.cluster ())
            {
                if (s.inbound ())
                    m_in_active += n;
                else
                    m_out_active += n;
            }
            m_active += n;
            break;

        case Slot::closing:
            m_closingCount += n;
            break;

        default:
            assert (false);
            break;
        };
    }

private:
    
    int m_attempts;

    
    std::size_t m_active;

    
    std::size_t m_in_max;

    
    std::size_t m_in_active;

    
    std::size_t m_out_max;

    
    std::size_t m_out_active;

    
    std::size_t m_fixed;

    
    std::size_t m_fixed_active;

    
    std::size_t m_cluster;




    int m_acceptCount;

    int m_closingCount;

    
    double m_roundingThreshold;
};

}
}

#endif








