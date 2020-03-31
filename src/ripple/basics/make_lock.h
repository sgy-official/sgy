

#ifndef RIPPLE_BASICS_MAKE_LOCK_H_INCLUDED
#define RIPPLE_BASICS_MAKE_LOCK_H_INCLUDED

#include <mutex>
#include <utility>

namespace ripple {

template <class Mutex, class ...Args>
inline
std::unique_lock<Mutex>
make_lock(Mutex& mutex, Args&&... args)
{
    return std::unique_lock<Mutex>(mutex, std::forward<Args>(args)...);
}

} 

#endif








