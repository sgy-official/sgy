

#ifndef RIPPLE_NODESTORE_TASK_H_INCLUDED
#define RIPPLE_NODESTORE_TASK_H_INCLUDED

namespace ripple {
namespace NodeStore {


struct Task
{
    virtual ~Task() = default;

    
    virtual void performScheduledTask() = 0;
};

}
}

#endif








