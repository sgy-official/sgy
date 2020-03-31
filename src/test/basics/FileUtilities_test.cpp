

#include <ripple/basics/ByteUtilities.h>
#include <ripple/basics/FileUtilities.h>
#include <ripple/beast/unit_test.h>
#include <test/unit_test/FileDirGuard.h>

namespace ripple {

class FileUtilities_test : public beast::unit_test::suite
{
public:
    void testGetFileContents()
    {
        using namespace ripple::test::detail;
        using namespace boost::system;

        constexpr const char* expectedContents =
            "This file is very short. That's all we need.";

        FileDirGuard file(*this, "test_file", "test.txt",
            expectedContents);

        error_code ec;
        auto const path = file.file();

        {
            auto const good = getFileContents(ec, path);
            BEAST_EXPECT(!ec);
            BEAST_EXPECT(good == expectedContents);
        }

        {
            auto const good = getFileContents(ec, path, kilobytes(1));
            BEAST_EXPECT(!ec);
            BEAST_EXPECT(good == expectedContents);
        }

        {
            auto const bad = getFileContents(ec, path, 16);
            BEAST_EXPECT(ec && ec.value() ==
                boost::system::errc::file_too_large);
            BEAST_EXPECT(bad.empty());
        }

    }

    void run () override
    {
        testGetFileContents();
    }
};

BEAST_DEFINE_TESTSUITE(FileUtilities, ripple_basics, ripple);

} 
























