

#ifndef RIPPLE_NODESTORE_TYPES_H_INCLUDED
#define RIPPLE_NODESTORE_TYPES_H_INCLUDED

#include <ripple/nodestore/NodeObject.h>
#include <ripple/basics/BasicConfig.h>
#include <vector>

namespace ripple {
namespace NodeStore {

enum
{
    batchWritePreallocationSize = 256,

    batchWriteLimitSize = 65536
};


enum Status
{
    ok,
    notFound,
    dataCorrupt,
    unknown,

    customCode = 100
};


using Batch = std::vector <std::shared_ptr<NodeObject>>;

}
}

#endif








