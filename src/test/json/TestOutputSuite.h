

#ifndef RIPPLE_RPC_TESTOUTPUTSUITE_H_INCLUDED
#define RIPPLE_RPC_TESTOUTPUTSUITE_H_INCLUDED

#include <ripple/json/Output.h>
#include <ripple/json/Writer.h>
#include <test/jtx/TestSuite.h>

namespace ripple {
namespace test {

class TestOutputSuite : public TestSuite
{
protected:
    std::string output_;
    std::unique_ptr <Json::Writer> writer_;

    void setup (std::string const& testName)
    {
        testcase (testName);
        output_.clear ();
        writer_ = std::make_unique <Json::Writer> (
            Json::stringOutput (output_));
    }

    void expectResult (std::string const& expected,
                       std::string const& message = "")
    {
        writer_.reset ();

        expectEquals (output_, expected, message);
    }
};

} 
} 

#endif








