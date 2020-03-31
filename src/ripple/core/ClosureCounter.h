

#ifndef RIPPLE_CORE_CLOSURE_COUNTER_H_INCLUDED
#define RIPPLE_CORE_CLOSURE_COUNTER_H_INCLUDED

#include <ripple/basics/Log.h>
#include <boost/optional.hpp>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <type_traits>

namespace ripple {

template <typename Ret_t, typename... Args_t>
class ClosureCounter
{
private:
    std::mutex mutable mutex_ {};
    std::condition_variable allClosuresDoneCond_ {};  
    bool waitForClosures_ {false};                    
    std::atomic<int> closureCount_ {0};

    ClosureCounter& operator++()
    {
        ++closureCount_;
        return *this;
    }

    ClosureCounter&  operator--()
    {
        std::lock_guard<std::mutex> lock {mutex_};

        if ((--closureCount_ == 0) && waitForClosures_)
            allClosuresDoneCond_.notify_all();
        return *this;
    }

    template <typename Closure>
    class Wrapper
    {
    private:
        ClosureCounter& counter_;
        std::remove_reference_t<Closure> closure_;

        static_assert (
            std::is_same<decltype(
                closure_(std::declval<Args_t>()...)), Ret_t>::value,
                "Closure arguments don't match ClosureCounter Ret_t or Args_t");

    public:
        Wrapper() = delete;

        Wrapper (Wrapper const& rhs)
        : counter_ (rhs.counter_)
        , closure_ (rhs.closure_)
        {
            ++counter_;
        }

        Wrapper (Wrapper&& rhs) noexcept(
          std::is_nothrow_move_constructible<Closure>::value)
        : counter_ (rhs.counter_)
        , closure_ (std::move (rhs.closure_))
        {
            ++counter_;
        }

        Wrapper (ClosureCounter& counter, Closure&& closure)
        : counter_ (counter)
        , closure_ (std::forward<Closure> (closure))
        {
            ++counter_;
        }

        Wrapper& operator=(Wrapper const& rhs) = delete;
        Wrapper& operator=(Wrapper&& rhs) = delete;

        ~Wrapper()
        {
            --counter_;
        }

        Ret_t operator ()(Args_t... args)
        {
            return closure_ (std::forward<Args_t>(args)...);
        }
    };

public:
    ClosureCounter() = default;
    ClosureCounter (ClosureCounter const&) = delete;

    ClosureCounter& operator=(ClosureCounter const&) = delete;

    
    ~ClosureCounter()
    {
        using namespace std::chrono_literals;
        join ("ClosureCounter", 1s, debugLog());
    }

    
    void join (char const* name,
        std::chrono::milliseconds wait, beast::Journal j)
    {
        std::unique_lock<std::mutex> lock {mutex_};
        waitForClosures_ = true;
        if (closureCount_ > 0)
        {
            if (! allClosuresDoneCond_.wait_for (
                lock, wait, [this] { return closureCount_ == 0; }))
            {
                if (auto stream = j.error())
                    stream << name
                        << " waiting for ClosureCounter::join().";
                allClosuresDoneCond_.wait (
                    lock, [this] { return closureCount_ == 0; });
            }
        }
    }

    
    template <class Closure>
    boost::optional<Wrapper<Closure>>
    wrap (Closure&& closure)
    {
        boost::optional<Wrapper<Closure>> ret;

        std::lock_guard<std::mutex> lock {mutex_};
        if (! waitForClosures_)
            ret.emplace (*this, std::forward<Closure> (closure));

        return ret;
    }

    
    int count() const
    {
        return closureCount_;
    }

    
    bool joined() const
    {
        std::lock_guard<std::mutex> lock {mutex_};
        return waitForClosures_;
    }
};

} 

#endif 








