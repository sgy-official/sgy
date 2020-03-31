

#ifndef RIPPLE_NODESTORE_BACKEND_H_INCLUDED
#define RIPPLE_NODESTORE_BACKEND_H_INCLUDED

#include <ripple/nodestore/Types.h>

namespace ripple {
namespace NodeStore {


class Backend
{
public:
    
    virtual ~Backend() = default;

    
    virtual std::string getName() = 0;

    
    virtual void open(bool createIfMissing = true) = 0;

    
    virtual void close() = 0;

    
    virtual Status fetch (void const* key, std::shared_ptr<NodeObject>* pObject) = 0;

    
    virtual
    bool
    canFetchBatch() = 0;

    
    virtual
    std::vector<std::shared_ptr<NodeObject>>
    fetchBatch (std::size_t n, void const* const* keys) = 0;

    
    virtual void store (std::shared_ptr<NodeObject> const& object) = 0;

    
    virtual void storeBatch (Batch const& batch) = 0;

    
    virtual void for_each (std::function <void (std::shared_ptr<NodeObject>)> f) = 0;

    
    virtual int getWriteLoad () = 0;

    
    virtual void setDeletePath() = 0;

    
    virtual void verify() = 0;

    
    virtual int fdlimit() const = 0;

    
    bool
    backed() const
    {
        return fdlimit();
    }
};

}
}

#endif








