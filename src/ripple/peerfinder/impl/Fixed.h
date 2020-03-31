

#ifndef RIPPLE_PEERFINDER_FIXED_H_INCLUDED
#define RIPPLE_PEERFINDER_FIXED_H_INCLUDED

#include <ripple/peerfinder/impl/Tuning.h>

namespace ripple {
namespace PeerFinder {


class Fixed
{
public:
    explicit Fixed (clock_type& clock)
        : m_when (clock.now ())
        , m_failures (0)
    {
    }

    Fixed (Fixed const&) = default;

    
    clock_type::time_point const& when () const
    {
        return m_when;
    }

    
    void failure (clock_type::time_point const& now)
    {
        m_failures = std::min (m_failures + 1,
            Tuning::connectionBackoff.size() - 1);
        m_when = now + std::chrono::minutes (
            Tuning::connectionBackoff [m_failures]);
    }

    
    void success (clock_type::time_point const& now)
    {
        m_failures = 0;
        m_when = now;
    }

private:
    clock_type::time_point m_when;
    std::size_t m_failures;
};

}
}

#endif








