

#include <ripple/nodestore/DummyScheduler.h>

namespace ripple {
namespace NodeStore {

void
DummyScheduler::scheduleTask (Task& task)
{
    task.performScheduledTask();
}

void
DummyScheduler::scheduledTasksStopped ()
{
}

void
DummyScheduler::onFetch (const FetchReport& report)
{
}

void
DummyScheduler::onBatchWrite (const BatchWriteReport& report)
{
}

}
}
























