

#ifndef RIPPLE_BASICS_SCOPEDLOCK_H_INCLUDED
#define RIPPLE_BASICS_SCOPEDLOCK_H_INCLUDED

namespace ripple
{


template <class MutexType>
class GenericScopedUnlock
{
    MutexType& lock_;
public:
    
    explicit GenericScopedUnlock (MutexType& lock) noexcept
        : lock_ (lock)
    {
        lock.unlock();
    }

    GenericScopedUnlock (GenericScopedUnlock const&) = delete;
    GenericScopedUnlock& operator= (GenericScopedUnlock const&) = delete;

    
    ~GenericScopedUnlock() noexcept(false)
    {
        lock_.lock();
    }
};

} 
#endif









