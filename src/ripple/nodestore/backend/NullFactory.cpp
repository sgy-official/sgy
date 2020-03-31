

#include <ripple/basics/contract.h>
#include <ripple/nodestore/Factory.h>
#include <ripple/nodestore/Manager.h>
#include <memory>

namespace ripple {
namespace NodeStore {

class NullBackend : public Backend
{
public:
    NullBackend() = default;

    ~NullBackend() = default;

    std::string
    getName() override
    {
        return std::string ();
    }

    void
    open(bool createIfMissing) override
    {
    }

    void
    close() override
    {
    }

    Status
    fetch (void const*, std::shared_ptr<NodeObject>*) override
    {
        return notFound;
    }

    bool
    canFetchBatch() override
    {
        return false;
    }

    std::vector<std::shared_ptr<NodeObject>>
    fetchBatch (std::size_t n, void const* const* keys) override
    {
        Throw<std::runtime_error> ("pure virtual called");
        return {};
    }

    void
    store (std::shared_ptr<NodeObject> const& object) override
    {
    }

    void
    storeBatch (Batch const& batch) override
    {
    }

    void
    for_each (std::function <void(std::shared_ptr<NodeObject>)> f) override
    {
    }

    int
    getWriteLoad () override
    {
        return 0;
    }

    void
    setDeletePath() override
    {
    }

    void
    verify() override
    {
    }

    
    int
    fdlimit() const override
    {
        return 0;
    }

private:
};


class NullFactory : public Factory
{
public:
    NullFactory()
    {
        Manager::instance().insert(*this);
    }

    ~NullFactory() override
    {
        Manager::instance().erase(*this);
    }

    std::string
    getName () const override
    {
        return "none";
    }

    std::unique_ptr <Backend>
    createInstance (
        size_t,
        Section const&,
        Scheduler&, beast::Journal) override
    {
        return std::make_unique <NullBackend> ();
    }
};

static NullFactory nullFactory;

}
}
























