

#include <ripple/nodestore/impl/ManagerImp.h>
#include <ripple/nodestore/impl/DatabaseNodeImp.h>

namespace ripple {
namespace NodeStore {

ManagerImp&
ManagerImp::instance()
{
    static ManagerImp _;
    return _;
}

void
ManagerImp::missing_backend()
{
    Throw<std::runtime_error> (
        "Your rippled.cfg is missing a [node_db] entry, "
        "please see the rippled-example.cfg file!"
        );
}

std::unique_ptr <Backend>
ManagerImp::make_Backend (
    Section const& parameters,
    Scheduler& scheduler,
    beast::Journal journal)
{
    std::string const type {get<std::string>(parameters, "type")};
    if (type.empty())
        missing_backend();

    auto factory {find(type)};
    if(!factory)
        missing_backend();

    return factory->createInstance(
        NodeObject::keyBytes, parameters, scheduler, journal);
}

std::unique_ptr <Database>
ManagerImp::make_Database (
    std::string const& name,
    Scheduler& scheduler,
    int readThreads,
    Stoppable& parent,
    Section const& config,
    beast::Journal journal)
{
    auto backend {make_Backend(config, scheduler, journal)};
    backend->open();
    return std::make_unique <DatabaseNodeImp>(
        name,
        scheduler,
        readThreads,
        parent,
        std::move(backend),
        config,
        journal);
}

void
ManagerImp::insert (Factory& factory)
{
    std::lock_guard<std::mutex> _(mutex_);
    list_.push_back(&factory);
}

void
ManagerImp::erase (Factory& factory)
{
    std::lock_guard<std::mutex> _(mutex_);
    auto const iter = std::find_if(list_.begin(), list_.end(),
        [&factory](Factory* other) { return other == &factory; });
    assert(iter != list_.end());
    list_.erase(iter);
}

Factory*
ManagerImp::find (std::string const& name)
{
    std::lock_guard<std::mutex> _(mutex_);
    auto const iter = std::find_if(list_.begin(), list_.end(),
        [&name](Factory* other)
        {
            return boost::beast::detail::iequals(name, other->getName());
        } );
    if (iter == list_.end())
        return nullptr;
    return *iter;
}


Manager&
Manager::instance()
{
    return ManagerImp::instance();
}


std::unique_ptr <Backend>
make_Backend (Section const& config,
    Scheduler& scheduler, beast::Journal journal)
{
    return Manager::instance().make_Backend (
        config, scheduler, journal);
}

}
}
























