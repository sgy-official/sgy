

#include <ripple/app/main/NodeStoreScheduler.h>
#include <cassert>

namespace ripple {

NodeStoreScheduler::NodeStoreScheduler (Stoppable& parent)
    : Stoppable ("NodeStoreScheduler", parent)
{
}

void NodeStoreScheduler::setJobQueue (JobQueue& jobQueue)
{
    m_jobQueue = &jobQueue;
}

void NodeStoreScheduler::onStop ()
{
}

void NodeStoreScheduler::onChildrenStopped ()
{
    assert (m_taskCount == 0);
    stopped ();
}

void NodeStoreScheduler::scheduleTask (NodeStore::Task& task)
{
    ++m_taskCount;
    if (!m_jobQueue->addJob (
        jtWRITE,
        "NodeObject::store",
        [this, &task] (Job&) { doTask(task); }))
    {
        doTask (task);
    }
}

void NodeStoreScheduler::doTask (NodeStore::Task& task)
{
    task.performScheduledTask ();

    if ((--m_taskCount == 0) && isStopping() && areChildrenStopped())
        stopped();
}

void NodeStoreScheduler::onFetch (NodeStore::FetchReport const& report)
{
    if (report.wentToDisk)
        m_jobQueue->addLoadEvents (
            report.isAsync ? jtNS_ASYNC_READ : jtNS_SYNC_READ,
                1, report.elapsed);
}

void NodeStoreScheduler::onBatchWrite (NodeStore::BatchWriteReport const& report)
{
    m_jobQueue->addLoadEvents (jtNS_WRITE,
        report.writeCount, report.elapsed);
}

} 
























