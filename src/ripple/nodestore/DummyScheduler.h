

#ifndef RIPPLE_NODESTORE_DUMMYSCHEDULER_H_INCLUDED
#define RIPPLE_NODESTORE_DUMMYSCHEDULER_H_INCLUDED

#include <ripple/nodestore/Scheduler.h>

namespace ripple {
namespace NodeStore {


class DummyScheduler : public Scheduler
{
public:
    DummyScheduler () = default;
    ~DummyScheduler () = default;
    void scheduleTask (Task& task) override;
    void scheduledTasksStopped ();
    void onFetch (FetchReport const& report) override;
    void onBatchWrite (BatchWriteReport const& report) override;
};

}
}

#endif








