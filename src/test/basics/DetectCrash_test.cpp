

#include <beast/unit_test/suite.hpp>

#include <exception>

namespace ripple {
namespace test {

struct DetectCrash_test : public beast::unit_test::suite
{
    void testDetectCrash ()
    {
        testcase ("Detect Crash");
        std::terminate();
    }
    void run() override
    {
        testDetectCrash();
    }
};

BEAST_DEFINE_TESTSUITE_MANUAL(DetectCrash,unit_test,beast);

} 
} 
























