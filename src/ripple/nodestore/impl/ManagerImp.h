

#ifndef RIPPLE_NODESTORE_MANAGERIMP_H_INCLUDED
#define RIPPLE_NODESTORE_MANAGERIMP_H_INCLUDED

#include <ripple/nodestore/Manager.h>

namespace ripple {
namespace NodeStore {

class ManagerImp : public Manager
{
private:
    std::mutex mutex_;
    std::vector<Factory*> list_;

public:
    static
    ManagerImp&
    instance();

    static
    void
    missing_backend();

    ManagerImp() = default;

    ~ManagerImp() = default;

    Factory*
    find (std::string const& name) override;

    void
    insert (Factory& factory) override;

    void
    erase (Factory& factory) override;

    std::unique_ptr <Backend>
    make_Backend (
        Section const& parameters,
        Scheduler& scheduler,
        beast::Journal journal) override;

    std::unique_ptr <Database>
    make_Database (
        std::string const& name,
        Scheduler& scheduler,
        int readThreads,
        Stoppable& parent,
        Section const& config,
        beast::Journal journal) override;
};

}
}

#endif








