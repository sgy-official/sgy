

#ifndef RIPPLE_APP_MAIN_COLLECTORMANAGER_H_INCLUDED
#define RIPPLE_APP_MAIN_COLLECTORMANAGER_H_INCLUDED

#include <ripple/basics/BasicConfig.h>
#include <ripple/beast/insight/Insight.h>

namespace ripple {


class CollectorManager
{
public:
    static std::unique_ptr<CollectorManager> New (
        Section const& params, beast::Journal journal);

    virtual ~CollectorManager () = 0;
    virtual beast::insight::Collector::ptr const& collector () = 0;
    virtual beast::insight::Group::ptr const& group (
        std::string const& name) = 0;
};

}

#endif








