

#ifndef RIPPLE_SERVER_IO_LIST_H_INCLUDED
#define RIPPLE_SERVER_IO_LIST_H_INCLUDED

#include <boost/container/flat_map.hpp>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace ripple {


class io_list final
{
public:
    class work
    {
        template<class = void>
        void destroy();

        friend class io_list;
        io_list* ios_ = nullptr;

    public:
        virtual ~work()
        {
            destroy();
        }

        
        io_list&
        ios()
        {
            return *ios_;
        }

        virtual void close() = 0;
    };

private:
    template<class = void>
    void destroy();

    std::mutex m_;
    std::size_t n_ = 0;
    bool closed_ = false;
    std::condition_variable cv_;
    boost::container::flat_map<work*,
        std::weak_ptr<work>> map_;
    std::function<void(void)> f_;

public:
    io_list() = default;

    
    ~io_list()
    {
        destroy();
    }

    
    bool
    closed() const
    {
        return closed_;
    }

    
    template <class T, class... Args>
    std::shared_ptr<T>
    emplace(Args&&... args);

    
    template<class Finisher>
    void
    close(Finisher&& f);

    void
    close()
    {
        close([]{});
    }

    
    template<class = void>
    void
    join();
};


template<class>
void
io_list::work::destroy()
{
    if(! ios_)
        return;
    std::function<void(void)> f;
    {
        std::lock_guard<
            std::mutex> lock(ios_->m_);
        ios_->map_.erase(this);
        if(--ios_->n_ == 0 &&
            ios_->closed_)
        {
            std::swap(f, ios_->f_);
            ios_->cv_.notify_all();
        }
    }
    if(f)
        f();
}

template<class>
void
io_list::destroy()
{
    close();
    join();
}

template <class T, class... Args>
std::shared_ptr<T>
io_list::emplace(Args&&... args)
{
    static_assert(std::is_base_of<work, T>::value,
        "T must derive from io_list::work");
    if(closed_)
        return nullptr;
    auto sp = std::make_shared<T>(
        std::forward<Args>(args)...);
    decltype(sp) dead;

    std::lock_guard<std::mutex> lock(m_);
    if(! closed_)
    {
        ++n_;
        sp->work::ios_ = this;
        map_.emplace(sp.get(), sp);
    }
    else
    {
        std::swap(sp, dead);
    }
    return sp;
}

template<class Finisher>
void
io_list::close(Finisher&& f)
{
    std::unique_lock<std::mutex> lock(m_);
    if(closed_)
        return;
    closed_ = true;
    auto map = std::move(map_);
    if(! map.empty())
    {
        f_ = std::forward<Finisher>(f);
        lock.unlock();
        for(auto const& p : map)
            if(auto sp = p.second.lock())
                sp->close();
    }
    else
    {
        lock.unlock();
        f();
    }
}

template<class>
void
io_list::join()
{
    std::unique_lock<std::mutex> lock(m_);
    cv_.wait(lock,
        [&]
        {
            return closed_ && n_ == 0;
        });
}

} 

#endif








