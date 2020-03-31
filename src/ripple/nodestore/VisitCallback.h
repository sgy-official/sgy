

#ifndef RIPPLE_NODESTORE_VISITCALLBACK_H_INCLUDED
#define RIPPLE_NODESTORE_VISITCALLBACK_H_INCLUDED

namespace ripple {
namespace NodeStore {


struct VisitCallback
{
    virtual void visitObject (NodeObject::Ptr const& object) = 0;
};

}
}

#endif








