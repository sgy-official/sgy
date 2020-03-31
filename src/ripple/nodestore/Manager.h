

#ifndef RIPPLE_NODESTORE_MANAGER_H_INCLUDED
#define RIPPLE_NODESTORE_MANAGER_H_INCLUDED

#include <ripple/nodestore/Factory.h>
#include <ripple/nodestore/DatabaseRotating.h>
#include <ripple/nodestore/DatabaseShard.h>

namespace ripple {
namespace NodeStore {


class Manager
{
public:
    virtual ~Manager () = default;
    Manager() = default;
    Manager(Manager const&) = delete;
    Manager& operator=(Manager const&) = delete;

    
    static
    Manager&
    instance();

    
    virtual
    void
    insert (Factory& factory) = 0;

    
    virtual
    void
    erase (Factory& factory) = 0;

    
    virtual
    Factory*
    find(std::string const& name) = 0;

    
    virtual
    std::unique_ptr <Backend>
    make_Backend (Section const& parameters,
        Scheduler& scheduler, beast::Journal journal) = 0;

    
    virtual
    std::unique_ptr <Database>
    make_Database (std::string const& name, Scheduler& scheduler,
        int readThreads, Stoppable& parent,
            Section const& backendParameters,
                beast::Journal journal) = 0;
};



std::unique_ptr <Backend>
make_Backend (Section const& config,
    Scheduler& scheduler, beast::Journal journal);

}
}

#endif








