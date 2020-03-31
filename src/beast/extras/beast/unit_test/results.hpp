
#ifndef BEAST_UNIT_TEST_RESULTS_HPP
#define BEAST_UNIT_TEST_RESULTS_HPP

#include <beast/unit_test/detail/const_container.hpp>

#include <string>
#include <vector>

namespace beast {
namespace unit_test {


class case_results
{
public:
    
    struct test
    {
        explicit test(bool pass_)
            : pass(pass_)
        {
        }

        test(bool pass_, std::string const& reason_)
            : pass(pass_)
            , reason(reason_)
        {
        }

        bool pass;
        std::string reason;
    };

private:
    class tests_t
        : public detail::const_container <std::vector <test>>
    {
    private:
        std::size_t failed_;

    public:
        tests_t()
            : failed_(0)
        {
        }

        
        std::size_t
        total() const
        {
            return cont().size();
        }

        
        std::size_t
        failed() const
        {
            return failed_;
        }

        
        void
        pass()
        {
            cont().emplace_back(true);
        }

        
        void
        fail(std::string const& reason = "")
        {
            ++failed_;
            cont().emplace_back(false, reason);
        }
    };

    class log_t
        : public detail::const_container <std::vector <std::string>>
    {
    public:
        
        void
        insert(std::string const& s)
        {
            cont().push_back(s);
        }
    };

    std::string name_;

public:
    explicit case_results(std::string const& name = "")
        : name_(name)
    {
    }

    
    std::string const&
    name() const
    {
        return name_;
    }

    
    tests_t tests;

    
    log_t log;
};



class suite_results
    : public detail::const_container <std::vector <case_results>>
{
private:
    std::string name_;
    std::size_t total_ = 0;
    std::size_t failed_ = 0;

public:
    explicit suite_results(std::string const& name = "")
        : name_(name)
    {
    }

    
    std::string const&
    name() const
    {
        return name_;
    }

    
    std::size_t
    total() const
    {
        return total_;
    }

    
    std::size_t
    failed() const
    {
        return failed_;
    }

    
    
    void
    insert(case_results&& r)
    {
        cont().emplace_back(std::move(r));
        total_ += r.tests.total();
        failed_ += r.tests.failed();
    }

    void
    insert(case_results const& r)
    {
        cont().push_back(r);
        total_ += r.tests.total();
        failed_ += r.tests.failed();
    }
    
};



class results
    : public detail::const_container <std::vector <suite_results>>
{
private:
    std::size_t m_cases;
    std::size_t total_;
    std::size_t failed_;

public:
    results()
        : m_cases(0)
        , total_(0)
        , failed_(0)
    {
    }

    
    std::size_t
    cases() const
    {
        return m_cases;
    }

    
    std::size_t
    total() const
    {
        return total_;
    }

    
    std::size_t
    failed() const
    {
        return failed_;
    }

    
    
    void
    insert(suite_results&& r)
    {
        m_cases += r.size();
        total_ += r.total();
        failed_ += r.failed();
        cont().emplace_back(std::move(r));
    }

    void
    insert(suite_results const& r)
    {
        m_cases += r.size();
        total_ += r.total();
        failed_ += r.failed();
        cont().push_back(r);
    }
    
};

} 
} 

#endif






