

#include <ripple/app/ledger/PendingSaves.h>
#include <ripple/beast/unit_test.h>

namespace ripple {
namespace test {

struct PendingSaves_test : public beast::unit_test::suite
{
    void testSaves()
    {
        PendingSaves ps;

        BEAST_EXPECT(!ps.pending (0));
        BEAST_EXPECT(!ps.startWork(0));
        BEAST_EXPECT(ps.shouldWork (0, true));
        BEAST_EXPECT(ps.startWork (0));
        BEAST_EXPECT(ps.pending (0));
        BEAST_EXPECT(! ps.shouldWork (0, false));
        ps.finishWork(0);
        BEAST_EXPECT(!ps.pending (0));

        BEAST_EXPECT(ps.shouldWork (0, false));
        BEAST_EXPECT(ps.pending (0));
        BEAST_EXPECT(ps.shouldWork (0, true));
        BEAST_EXPECT(ps.pending (0));
        BEAST_EXPECT(ps.startWork (0));
        BEAST_EXPECT(!ps.startWork (0));
        ps.finishWork(0);
        BEAST_EXPECT(! ps.pending (0));
    }

    void run() override
    {
        testSaves();
    }
};

BEAST_DEFINE_TESTSUITE(PendingSaves,ledger,ripple);

} 
} 
























