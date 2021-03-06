
#ifndef BEAST_UNIT_TEST_THREAD_HPP
#define BEAST_UNIT_TEST_THREAD_HPP

#include <beast/unit_test/suite.hpp>
#include <functional>
#include <thread>
#include <utility>

namespace beast {
namespace unit_test {


class thread
{
private:
    suite* s_ = nullptr;
    std::thread t_;

public:
    using id = std::thread::id;
    using native_handle_type = std::thread::native_handle_type;

    thread() = default;
    thread(thread const&) = delete;
    thread& operator=(thread const&) = delete;

    thread(thread&& other)
        : s_(other.s_)
        , t_(std::move(other.t_))
    {
    }

    thread& operator=(thread&& other)
    {
        s_ = other.s_;
        t_ = std::move(other.t_);
        return *this;
    }

    template<class F, class... Args>
    explicit
    thread(suite& s, F&& f, Args&&... args)
        : s_(&s)
    {
        std::function<void(void)> b =
            std::bind(std::forward<F>(f),
                std::forward<Args>(args)...);
        t_ = std::thread(&thread::run, this,
            std::move(b));
    }

    bool
    joinable() const
    {
        return t_.joinable();
    }

    std::thread::id
    get_id() const
    {
        return t_.get_id();
    }

    static
    unsigned
    hardware_concurrency() noexcept
    {
        return std::thread::hardware_concurrency();
    }

    void
    join()
    {
        t_.join();
        s_->propagate_abort();
    }

    void
    detach()
    {
        t_.detach();
    }

    void
    swap(thread& other)
    {
        std::swap(s_, other.s_);
        std::swap(t_, other.t_);
    }

private:
    void
    run(std::function <void(void)> f)
    {
        try
        {
            f();
        }
        catch(suite::abort_exception const&)
        {
        }
        catch(std::exception const& e)
        {
            s_->fail("unhandled exception: " +
                std::string(e.what()));
        }
        catch(...)
        {
            s_->fail("unhandled exception");
        }
    }
};

} 
} 

#endif






