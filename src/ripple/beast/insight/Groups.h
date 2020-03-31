

#ifndef BEAST_INSIGHT_GROUPS_H_INCLUDED
#define BEAST_INSIGHT_GROUPS_H_INCLUDED

#include <ripple/beast/insight/Collector.h>
#include <ripple/beast/insight/Group.h>

#include <memory>
#include <string>

namespace beast {
namespace insight {


class Groups
{
public:
    virtual ~Groups() = 0;

    
    
    virtual
    Group::ptr const&
    get (std::string const& name) = 0;

    Group::ptr const&
    operator[] (std::string const& name)
    {
        return get (name);
    }
    
};


std::unique_ptr <Groups> make_Groups (Collector::ptr const& collector);

}
}

#endif








