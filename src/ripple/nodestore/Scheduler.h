

#ifndef RIPPLE_NODESTORE_SCHEDULER_H_INCLUDED
#define RIPPLE_NODESTORE_SCHEDULER_H_INCLUDED

#include <ripple/nodestore/Task.h>
#include <chrono>

namespace ripple {
namespace NodeStore {


struct FetchReport
{
    explicit FetchReport() = default;

    std::chrono::milliseconds elapsed;
    bool isAsync;
    bool wentToDisk;
    bool wasFound;
};


struct BatchWriteReport
{
    explicit BatchWriteReport() = default;

    std::chrono::milliseconds elapsed;
    int writeCount;
};


class Scheduler
{
public:
    virtual ~Scheduler() = default;

    
    virtual void scheduleTask (Task& task) = 0;

    
    virtual void onFetch (FetchReport const& report) = 0;

    
    virtual void onBatchWrite (BatchWriteReport const& report) = 0;
};

}
}

#endif








