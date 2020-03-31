

#ifndef RIPPLE_NODESTORE_DATABASEROTATING_H_INCLUDED
#define RIPPLE_NODESTORE_DATABASEROTATING_H_INCLUDED

#include <ripple/nodestore/Database.h>

namespace ripple {
namespace NodeStore {



class DatabaseRotating : public Database
{
public:
    DatabaseRotating(
        std::string const& name,
        Stoppable& parent,
        Scheduler& scheduler,
        int readThreads,
        Section const& config,
        beast::Journal journal)
        : Database(name, parent, scheduler, readThreads, config, journal)
    {}

    virtual
    TaggedCache<uint256, NodeObject> const&
    getPositiveCache() = 0;

    virtual std::mutex& peekMutex() const = 0;

    virtual
    std::unique_ptr<Backend> const&
    getWritableBackend() const = 0;

    virtual
    std::unique_ptr<Backend>
    rotateBackends(std::unique_ptr<Backend> newBackend) = 0;
};

}
}

#endif








