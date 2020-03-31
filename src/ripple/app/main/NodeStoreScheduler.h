

#ifndef RIPPLE_APP_MAIN_NODESTORESCHEDULER_H_INCLUDED
#define RIPPLE_APP_MAIN_NODESTORESCHEDULER_H_INCLUDED

#include <ripple/nodestore/Scheduler.h>
#include <ripple/core/JobQueue.h>
#include <ripple/core/Stoppable.h>
#include <atomic>

namespace ripple {


class NodeStoreScheduler
    : public NodeStore::Scheduler
    , public Stoppable
{
public:
    NodeStoreScheduler (Stoppable& parent);

    void setJobQueue (JobQueue& jobQueue);

    void onStop () override;
    void onChildrenStopped () override;
    void scheduleTask (NodeStore::Task& task) override;
    void onFetch (NodeStore::FetchReport const& report) override;
    void onBatchWrite (NodeStore::BatchWriteReport const& report) override;

private:
    void doTask (NodeStore::Task& task);

    JobQueue* m_jobQueue {nullptr};
    std::atomic <int> m_taskCount {0};
};

} 

#endif








