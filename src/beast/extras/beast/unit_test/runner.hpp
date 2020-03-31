
#ifndef BEAST_UNIT_TEST_RUNNER_H_INCLUDED
#define BEAST_UNIT_TEST_RUNNER_H_INCLUDED

#include <beast/unit_test/suite_info.hpp>
#include <boost/assert.hpp>
#include <mutex>
#include <ostream>
#include <string>

namespace beast {
namespace unit_test {


class runner
{
    std::string arg_;
    bool default_ = false;
    bool failed_ = false;
    bool cond_ = false;
    std::recursive_mutex mutex_;

public:
    runner() = default;
    virtual ~runner() = default;
    runner(runner const&) = delete;
    runner& operator=(runner const&) = delete;

    
    void
    arg(std::string const& s)
    {
        arg_ = s;
    }

    
    std::string const&
    arg() const
    {
        return arg_;
    }

    
    template<class = void>
    bool
    run(suite_info const& s);

    
    template<class FwdIter>
    bool
    run(FwdIter first, FwdIter last);

    
    template<class FwdIter, class Pred>
    bool
    run_if(FwdIter first, FwdIter last, Pred pred = Pred{});

    
    template<class SequenceContainer>
    bool
    run_each(SequenceContainer const& c);

    
    template<class SequenceContainer, class Pred>
    bool
    run_each_if(SequenceContainer const& c, Pred pred = Pred{});

protected:
    virtual
    void
    on_suite_begin(suite_info const&)
    {
    }

    virtual
    void
    on_suite_end()
    {
    }

    virtual
    void
    on_case_begin(std::string const&)
    {
    }

    virtual
    void
    on_case_end()
    {
    }

    virtual
    void
    on_pass()
    {
    }

    virtual
    void
    on_fail(std::string const&)
    {
    }

    virtual
    void
    on_log(std::string const&)
    {
    }

private:
    friend class suite;

    template<class = void>
    void
    testcase(std::string const& name);

    template<class = void>
    void
    pass();

    template<class = void>
    void
    fail(std::string const& reason);

    template<class = void>
    void
    log(std::string const& s);
};


template<class>
bool
runner::run(suite_info const& s)
{
    default_ = true;
    failed_ = false;
    on_suite_begin(s);
    s.run(*this);
    BOOST_ASSERT(cond_);
    on_case_end();
    on_suite_end();
    return failed_;
}

template<class FwdIter>
bool
runner::run(FwdIter first, FwdIter last)
{
    bool failed(false);
    for(;first != last; ++first)
        failed = run(*first) || failed;
    return failed;
}

template<class FwdIter, class Pred>
bool
runner::run_if(FwdIter first, FwdIter last, Pred pred)
{
    bool failed(false);
    for(;first != last; ++first)
        if(pred(*first))
            failed = run(*first) || failed;
    return failed;
}

template<class SequenceContainer>
bool
runner::run_each(SequenceContainer const& c)
{
    bool failed(false);
    for(auto const& s : c)
        failed = run(s) || failed;
    return failed;
}

template<class SequenceContainer, class Pred>
bool
runner::run_each_if(SequenceContainer const& c, Pred pred)
{
    bool failed(false);
    for(auto const& s : c)
        if(pred(s))
            failed = run(s) || failed;
    return failed;
}

template<class>
void
runner::testcase(std::string const& name)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    BOOST_ASSERT(default_ || ! name.empty());
    BOOST_ASSERT(default_ || cond_);
    if(! default_)
        on_case_end();
    default_ = false;
    cond_ = false;
    on_case_begin(name);
}

template<class>
void
runner::pass()
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if(default_)
        testcase("");
    on_pass();
    cond_ = true;
}

template<class>
void
runner::fail(std::string const& reason)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if(default_)
        testcase("");
    on_fail(reason);
    failed_ = true;
    cond_ = true;
}

template<class>
void
runner::log(std::string const& s)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if(default_)
        testcase("");
    on_log(s);
}

} 
} 

#endif






