

#ifndef RIPPLE_CORE_SEMAPHORE_H_INCLUDED
#define RIPPLE_CORE_SEMAPHORE_H_INCLUDED

#include <condition_variable>
#include <mutex>

namespace ripple {

template <class Mutex, class CondVar>
class basic_semaphore
{
private:
    using scoped_lock = std::unique_lock <Mutex>;

    Mutex m_mutex;
    CondVar m_cond;
    std::size_t m_count;

public:
    using size_type = std::size_t;

    
    explicit basic_semaphore (size_type count = 0)
        : m_count (count)
    {
    }

    
    void notify ()
    {
        scoped_lock lock (m_mutex);
        ++m_count;
        m_cond.notify_one ();
    }

    
    void wait ()
    {
        scoped_lock lock (m_mutex);
        while (m_count == 0)
            m_cond.wait (lock);
        --m_count;
    }

    
    bool try_wait ()
    {
        scoped_lock lock (m_mutex);
        if (m_count == 0)
            return false;
        --m_count;
        return true;
    }
};

using semaphore = basic_semaphore <std::mutex, std::condition_variable>;

} 

#endif









