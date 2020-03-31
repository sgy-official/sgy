
#include <beast/unit_test/amount.hpp>
#include <beast/unit_test/global_suites.hpp>
#include <beast/unit_test/suite.hpp>
#include <string>


namespace beast {
namespace unit_test {


class print_test : public suite
{
public:
    void
    run() override
    {
        std::size_t manual = 0;
        std::size_t total = 0;

        auto prefix = [](suite_info const& s)
        {
            return s.manual() ? "|M| " : "    ";
        };

        for (auto const& s : global_suites())
        {
            log << prefix (s) << s.full_name() << '\n';

            if (s.manual())
                ++manual;
            ++total;
        }

        log <<
            amount (total, "suite") << " total, " <<
            amount (manual, "manual suite") << std::endl;

        pass();
    }
};

BEAST_DEFINE_TESTSUITE_MANUAL(print,unit_test,beast);

} 
} 
























