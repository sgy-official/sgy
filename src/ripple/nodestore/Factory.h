

#ifndef RIPPLE_NODESTORE_FACTORY_H_INCLUDED
#define RIPPLE_NODESTORE_FACTORY_H_INCLUDED

#include <ripple/nodestore/Backend.h>
#include <ripple/nodestore/Scheduler.h>
#include <ripple/beast/utility/Journal.h>
#include <nudb/store.hpp>

namespace ripple {
namespace NodeStore {


class Factory
{
public:
    virtual
    ~Factory() = default;

    
    virtual
    std::string
    getName() const = 0;

    
    virtual
    std::unique_ptr <Backend>
    createInstance (
        size_t keyBytes,
        Section const& parameters,
        Scheduler& scheduler,
        beast::Journal journal) = 0;

    
    virtual
    std::unique_ptr <Backend>
    createInstance (
        size_t keyBytes,
        Section const& parameters,
        Scheduler& scheduler,
        nudb::context& context,
        beast::Journal journal)
    {
        return {};
    }
};

}
}

#endif








