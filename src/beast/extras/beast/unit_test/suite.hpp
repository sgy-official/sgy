
#ifndef BEAST_UNIT_TEST_SUITE_HPP
#define BEAST_UNIT_TEST_SUITE_HPP

#include <beast/unit_test/runner.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/throw_exception.hpp>
#include <ostream>
#include <sstream>
#include <string>

namespace beast {
namespace unit_test {

namespace detail {

template<class String>
static
std::string
make_reason(String const& reason,
    char const* file, int line)
{
    std::string s(reason);
    if(! s.empty())
        s.append(": ");
    namespace fs = boost::filesystem;
    s.append(fs::path{file}.filename().string());
    s.append("(");
    s.append(boost::lexical_cast<std::string>(line));
    s.append(")");
    return s;
}

} 

class thread;

enum abort_t
{
    no_abort_on_fail,
    abort_on_fail
};


class suite
{
private:
    bool abort_ = false;
    bool aborted_ = false;
    runner* runner_ = nullptr;

    struct abort_exception : public std::exception
    {
        char const*
        what() const noexcept override
        {
            return "test suite aborted";
        }
    };

    template<class CharT, class Traits, class Allocator>
    class log_buf
        : public std::basic_stringbuf<CharT, Traits, Allocator>
    {
        suite& suite_;

    public:
        explicit
        log_buf(suite& self)
            : suite_(self)
        {
        }

        ~log_buf()
        {
            sync();
        }

        int
        sync() override
        {
            auto const& s = this->str();
            if(s.size() > 0)
                suite_.runner_->log(s);
            this->str("");
            return 0;
        }
    };

    template<
        class CharT,
        class Traits = std::char_traits<CharT>,
        class Allocator = std::allocator<CharT>
    >
    class log_os : public std::basic_ostream<CharT, Traits>
    {
        log_buf<CharT, Traits, Allocator> buf_;

    public:
        explicit
        log_os(suite& self)
            : std::basic_ostream<CharT, Traits>(&buf_)
            , buf_(self)
        {
        }
    };

    class scoped_testcase;

    class testcase_t
    {
        suite& suite_;
        std::stringstream ss_;

    public:
        explicit
        testcase_t(suite& self)
            : suite_(self)
        {
        }

        
        void
        operator()(std::string const& name,
            abort_t abort = no_abort_on_fail);

        scoped_testcase
        operator()(abort_t abort);

        template<class T>
        scoped_testcase
        operator<<(T const& t);
    };

public:
    
    log_os<char> log;

    
    testcase_t testcase;

    
    static
    suite*
    this_suite()
    {
        return *p_this_suite();
    }

    suite()
        : log(*this)
        , testcase(*this)
    {
    }

    virtual ~suite() = default;
    suite(suite const&) = delete;
    suite& operator=(suite const&) = delete;

    
    template<class = void>
    void
    operator()(runner& r);

    
    template<class = void>
    void
    pass();

    
    
    template<class String>
    void
    fail(String const& reason, char const* file, int line);

    template<class = void>
    void
    fail(std::string const& reason = "");
    

    
    
    template<class Condition>
    bool
    expect(Condition const& shouldBeTrue)
    {
        return expect(shouldBeTrue, "");
    }

    template<class Condition, class String>
    bool
    expect(Condition const& shouldBeTrue, String const& reason);

    template<class Condition>
    bool
    expect(Condition const& shouldBeTrue,
        char const* file, int line)
    {
        return expect(shouldBeTrue, "", file, line);
    }

    template<class Condition, class String>
    bool
    expect(Condition const& shouldBeTrue,
        String const& reason, char const* file, int line);
    

    template<class F, class String>
    bool
    except(F&& f, String const& reason);
    template<class F>
    bool
    except(F&& f)
    {
        return except(f, "");
    }
    template<class E, class F, class String>
    bool
    except(F&& f, String const& reason);
    template<class E, class F>
    bool
    except(F&& f)
    {
        return except<E>(f, "");
    }
    template<class F, class String>
    bool
    unexcept(F&& f, String const& reason);
    template<class F>
    bool
    unexcept(F&& f)
    {
        return unexcept(f, "");
    }

    
    std::string const&
    arg() const
    {
        return runner_->arg();
    }

    template<class Condition, class String>
    bool
    unexpected(Condition shouldBeFalse,
        String const& reason);

    template<class Condition>
    bool
    unexpected(Condition shouldBeFalse)
    {
        return unexpected(shouldBeFalse, "");
    }

private:
    friend class thread;

    static
    suite**
    p_this_suite()
    {
        static suite* pts = nullptr;
        return &pts;
    }

    
    virtual
    void
    run() = 0;

    void
    propagate_abort();

    template<class = void>
    void
    run(runner& r);
};


class suite::scoped_testcase
{
private:
    suite& suite_;
    std::stringstream& ss_;

public:
    scoped_testcase& operator=(scoped_testcase const&) = delete;

    ~scoped_testcase()
    {
        auto const& name = ss_.str();
        if(! name.empty())
            suite_.runner_->testcase(name);
    }

    scoped_testcase(suite& self, std::stringstream& ss)
        : suite_(self)
        , ss_(ss)
    {
        ss_.clear();
        ss_.str({});
    }

    template<class T>
    scoped_testcase(suite& self,
            std::stringstream& ss, T const& t)
        : suite_(self)
        , ss_(ss)
    {
        ss_.clear();
        ss_.str({});
        ss_ << t;
    }

    template<class T>
    scoped_testcase&
    operator<<(T const& t)
    {
        ss_ << t;
        return *this;
    }
};


inline
void
suite::testcase_t::operator()(
    std::string const& name, abort_t abort)
{
    suite_.abort_ = abort == abort_on_fail;
    suite_.runner_->testcase(name);
}

inline
suite::scoped_testcase
suite::testcase_t::operator()(abort_t abort)
{
    suite_.abort_ = abort == abort_on_fail;
    return { suite_, ss_ };
}

template<class T>
inline
suite::scoped_testcase
suite::testcase_t::operator<<(T const& t)
{
    return { suite_, ss_, t };
}


template<class>
void
suite::
operator()(runner& r)
{
    *p_this_suite() = this;
    try
    {
        run(r);
        *p_this_suite() = nullptr;
    }
    catch(...)
    {
        *p_this_suite() = nullptr;
        throw;
    }
}

template<class Condition, class String>
bool
suite::
expect(
    Condition const& shouldBeTrue, String const& reason)
{
    if(shouldBeTrue)
    {
        pass();
        return true;
    }
    fail(reason);
    return false;
}

template<class Condition, class String>
bool
suite::
expect(Condition const& shouldBeTrue,
    String const& reason, char const* file, int line)
{
    if(shouldBeTrue)
    {
        pass();
        return true;
    }
    fail(detail::make_reason(reason, file, line));
    return false;
}


template<class F, class String>
bool
suite::
except(F&& f, String const& reason)
{
    try
    {
        f();
        fail(reason);
        return false;
    }
    catch(...)
    {
        pass();
    }
    return true;
}

template<class E, class F, class String>
bool
suite::
except(F&& f, String const& reason)
{
    try
    {
        f();
        fail(reason);
        return false;
    }
    catch(E const&)
    {
        pass();
    }
    return true;
}

template<class F, class String>
bool
suite::
unexcept(F&& f, String const& reason)
{
    try
    {
        f();
        pass();
        return true;
    }
    catch(...)
    {
        fail(reason);
    }
    return false;
}

template<class Condition, class String>
bool
suite::
unexpected(
    Condition shouldBeFalse, String const& reason)
{
    bool const b =
        static_cast<bool>(shouldBeFalse);
    if(! b)
        pass();
    else
        fail(reason);
    return ! b;
}

template<class>
void
suite::
pass()
{
    propagate_abort();
    runner_->pass();
}

template<class>
void
suite::
fail(std::string const& reason)
{
    propagate_abort();
    runner_->fail(reason);
    if(abort_)
    {
        aborted_ = true;
        BOOST_THROW_EXCEPTION(abort_exception());
    }
}

template<class String>
void
suite::
fail(String const& reason, char const* file, int line)
{
    fail(detail::make_reason(reason, file, line));
}

inline
void
suite::
propagate_abort()
{
    if(abort_ && aborted_)
        BOOST_THROW_EXCEPTION(abort_exception());
}

template<class>
void
suite::
run(runner& r)
{
    runner_ = &r;

    try
    {
        run();
    }
    catch(abort_exception const&)
    {
    }
    catch(std::exception const& e)
    {
        runner_->fail("unhandled exception: " +
            std::string(e.what()));
    }
    catch(...)
    {
        runner_->fail("unhandled exception");
    }
}

#ifndef BEAST_EXPECT

#define BEAST_EXPECT(cond) expect(cond, __FILE__, __LINE__)
#endif

#ifndef BEAST_EXPECTS

#define BEAST_EXPECTS(cond, reason) ((cond) ? (pass(), true) : \
        (fail((reason), __FILE__, __LINE__), false))
#endif

} 
} 


#define BEAST_DEFINE_TESTSUITE_INSERT(Class,Module,Library,manual,priority) \
    static beast::unit_test::detail::insert_suite <Class##_test>   \
        Library ## Module ## Class ## _test_instance(             \
            #Class, #Module, #Library, manual, priority)



#ifndef BEAST_DEFINE_TESTSUITE


#ifndef BEAST_NO_UNIT_TEST_INLINE
#define BEAST_NO_UNIT_TEST_INLINE 0
#endif



#if BEAST_NO_UNIT_TEST_INLINE
#define BEAST_DEFINE_TESTSUITE(Class,Module,Library)
#define BEAST_DEFINE_TESTSUITE_MANUAL(Class,Module,Library)
#define BEAST_DEFINE_TESTSUITE_PRIO(Class,Module,Library,Priority)
#define BEAST_DEFINE_TESTSUITE_MANUAL_PRIO(Class,Module,Library,Priority)

#else
#include <beast/unit_test/global_suites.hpp>
#define BEAST_DEFINE_TESTSUITE(Class,Module,Library) \
        BEAST_DEFINE_TESTSUITE_INSERT(Class,Module,Library,false,0)
#define BEAST_DEFINE_TESTSUITE_MANUAL(Class,Module,Library) \
        BEAST_DEFINE_TESTSUITE_INSERT(Class,Module,Library,true,0)
#define BEAST_DEFINE_TESTSUITE_PRIO(Class,Module,Library,Priority) \
        BEAST_DEFINE_TESTSUITE_INSERT(Class,Module,Library,false,Priority)
#define BEAST_DEFINE_TESTSUITE_MANUAL_PRIO(Class,Module,Library,Priority) \
        BEAST_DEFINE_TESTSUITE_INSERT(Class,Module,Library,true,Priority)
#endif

#endif


#endif






