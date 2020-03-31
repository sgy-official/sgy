

#ifndef RIPPLE_NODESTORE_BATCHWRITER_H_INCLUDED
#define RIPPLE_NODESTORE_BATCHWRITER_H_INCLUDED

#include <ripple/nodestore/Scheduler.h>
#include <ripple/nodestore/Task.h>
#include <ripple/nodestore/Types.h>
#include <condition_variable>
#include <mutex>

namespace ripple {
namespace NodeStore {


class BatchWriter : private Task
{
public:
    
    struct Callback
    {
        virtual ~Callback () = default;
        Callback() = default;
        Callback(Callback const&) = delete;
        Callback& operator=(Callback const&) = delete;

        virtual void writeBatch (Batch const& batch) = 0;
    };

    
    BatchWriter (Callback& callback, Scheduler& scheduler);

    
    ~BatchWriter ();

    
    void store (std::shared_ptr<NodeObject> const& object);

    
    int getWriteLoad ();

private:
    void performScheduledTask () override;
    void writeBatch ();
    void waitForWriting ();

private:
    using LockType = std::recursive_mutex;
    using CondvarType = std::condition_variable_any;

    Callback& m_callback;
    Scheduler& m_scheduler;
    LockType mWriteMutex;
    CondvarType mWriteCondition;
    int mWriteLoad;
    bool mWritePending;
    Batch mWriteSet;
};

}
}

#endif








