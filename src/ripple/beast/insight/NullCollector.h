

#ifndef BEAST_INSIGHT_NULLCOLLECTOR_H_INCLUDED
#define BEAST_INSIGHT_NULLCOLLECTOR_H_INCLUDED

#include <ripple/beast/insight/Collector.h>

namespace beast {
namespace insight {


class NullCollector : public Collector
{
public:
    explicit NullCollector() = default;

    static std::shared_ptr <Collector> New ();
};

}
}

#endif








