

#ifndef BEAST_INSIGHT_STATSDCOLLECTOR_H_INCLUDED
#define BEAST_INSIGHT_STATSDCOLLECTOR_H_INCLUDED

#include <ripple/beast/insight/Collector.h>

#include <ripple/beast/utility/Journal.h>
#include <ripple/beast/net/IPEndpoint.h>

namespace beast {
namespace insight {


class StatsDCollector : public Collector
{
public:
    explicit StatsDCollector() = default;

    
    static
    std::shared_ptr <StatsDCollector>
    New (IP::Endpoint const& address,
        std::string const& prefix, Journal journal);
};

}
}

#endif








