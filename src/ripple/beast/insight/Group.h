

#ifndef BEAST_INSIGHT_GROUP_H_INCLUDED
#define BEAST_INSIGHT_GROUP_H_INCLUDED

#include <ripple/beast/insight/Collector.h>

#include <memory>
#include <string>

namespace beast {
namespace insight {


class Group : public Collector
{
public:
    using ptr = std::shared_ptr <Group>;

    
    virtual std::string const& name () const = 0;
};

}
}

#endif








