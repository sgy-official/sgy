

#include <ripple/app/misc/HashRouter.h>
#include <ripple/basics/chrono.h>
#include <ripple/beast/unit_test.h>

namespace ripple {
namespace test {

class HashRouter_test : public beast::unit_test::suite
{
    void
    testNonExpiration()
    {
        using namespace std::chrono_literals;
        TestStopwatch stopwatch;
        HashRouter router(stopwatch, 2s, 2);

        uint256 const key1(1);
        uint256 const key2(2);
        uint256 const key3(3);

        router.setFlags(key1, 11111);
        BEAST_EXPECT(router.getFlags(key1) == 11111);
        router.setFlags(key2, 22222);
        BEAST_EXPECT(router.getFlags(key2) == 22222);

        ++stopwatch;

        BEAST_EXPECT(router.getFlags(key1) == 11111);

        ++stopwatch;

        router.setFlags(key3,33333); 
        BEAST_EXPECT(router.getFlags(key1) == 11111);
        BEAST_EXPECT(router.getFlags(key2) == 0);
    }

    void
    testExpiration()
    {
        using namespace std::chrono_literals;
        TestStopwatch stopwatch;
        HashRouter router(stopwatch, 2s, 2);

        uint256 const key1(1);
        uint256 const key2(2);
        uint256 const key3(3);
        uint256 const key4(4);
        BEAST_EXPECT(key1 != key2 &&
            key2 != key3 &&
            key3 != key4);

        router.setFlags(key1, 12345);
        BEAST_EXPECT(router.getFlags(key1) == 12345);

        ++stopwatch;

        router.setFlags(key2, 9999);
        BEAST_EXPECT(router.getFlags(key1) == 12345);
        BEAST_EXPECT(router.getFlags(key2) == 9999);

        ++stopwatch;
        BEAST_EXPECT(router.getFlags(key2) == 9999);

        ++stopwatch;
        router.setFlags(key3, 2222);
        BEAST_EXPECT(router.getFlags(key1) == 0);
        BEAST_EXPECT(router.getFlags(key2) == 9999);
        BEAST_EXPECT(router.getFlags(key3) == 2222);

        ++stopwatch;
        router.setFlags(key1, 7654);
        BEAST_EXPECT(router.getFlags(key1) == 7654);
        BEAST_EXPECT(router.getFlags(key2) == 9999);
        BEAST_EXPECT(router.getFlags(key3) == 2222);

        ++stopwatch;
        ++stopwatch;

        router.setFlags(key4, 7890);
        BEAST_EXPECT(router.getFlags(key1) == 0);
        BEAST_EXPECT(router.getFlags(key2) == 0);
        BEAST_EXPECT(router.getFlags(key3) == 0);
        BEAST_EXPECT(router.getFlags(key4) == 7890);
    }

    void testSuppression()
    {
        using namespace std::chrono_literals;
        TestStopwatch stopwatch;
        HashRouter router(stopwatch, 2s, 2);

        uint256 const key1(1);
        uint256 const key2(2);
        uint256 const key3(3);
        uint256 const key4(4);
        BEAST_EXPECT(key1 != key2 &&
            key2 != key3 &&
            key3 != key4);

        int flags = 12345;  
        router.addSuppression(key1);
        BEAST_EXPECT(router.addSuppressionPeer(key2, 15));
        BEAST_EXPECT(router.addSuppressionPeer(key3, 20, flags));
        BEAST_EXPECT(flags == 0);

        ++stopwatch;

        BEAST_EXPECT(!router.addSuppressionPeer(key1, 2));
        BEAST_EXPECT(!router.addSuppressionPeer(key2, 3));
        BEAST_EXPECT(!router.addSuppressionPeer(key3, 4, flags));
        BEAST_EXPECT(flags == 0);
        BEAST_EXPECT(router.addSuppressionPeer(key4, 5));
    }

    void
    testSetFlags()
    {
        using namespace std::chrono_literals;
        TestStopwatch stopwatch;
        HashRouter router(stopwatch, 2s, 2);

        uint256 const key1(1);
        BEAST_EXPECT(router.setFlags(key1, 10));
        BEAST_EXPECT(!router.setFlags(key1, 10));
        BEAST_EXPECT(router.setFlags(key1, 20));
    }

    void
    testRelay()
    {
        using namespace std::chrono_literals;
        TestStopwatch stopwatch;
        HashRouter router(stopwatch, 1s, 2);

        uint256 const key1(1);

        boost::optional<std::set<HashRouter::PeerShortID>> peers;

        peers = router.shouldRelay(key1);
        BEAST_EXPECT(peers && peers->empty());
        router.addSuppressionPeer(key1, 1);
        router.addSuppressionPeer(key1, 3);
        router.addSuppressionPeer(key1, 5);
        BEAST_EXPECT(!router.shouldRelay(key1));
        ++stopwatch;
        peers = router.shouldRelay(key1);
        BEAST_EXPECT(peers && peers->size() == 3);
        router.addSuppressionPeer(key1, 2);
        router.addSuppressionPeer(key1, 4);
        BEAST_EXPECT(!router.shouldRelay(key1));
        ++stopwatch;
        peers = router.shouldRelay(key1);
        BEAST_EXPECT(peers && peers->size() == 2);
        ++stopwatch;
        peers = router.shouldRelay(key1);
        BEAST_EXPECT(peers && peers->size() == 0);
    }

    void
    testRecover()
    {
        using namespace std::chrono_literals;
        TestStopwatch stopwatch;
        HashRouter router(stopwatch, 1s, 5);

        uint256 const key1(1);

        BEAST_EXPECT(router.shouldRecover(key1));
        BEAST_EXPECT(router.shouldRecover(key1));
        BEAST_EXPECT(router.shouldRecover(key1));
        BEAST_EXPECT(router.shouldRecover(key1));
        BEAST_EXPECT(router.shouldRecover(key1));
        BEAST_EXPECT(!router.shouldRecover(key1));
        ++stopwatch;
        BEAST_EXPECT(router.shouldRecover(key1));
        ++stopwatch;
        BEAST_EXPECT(router.shouldRecover(key1));
        BEAST_EXPECT(router.shouldRecover(key1));
        BEAST_EXPECT(router.shouldRecover(key1));
        ++stopwatch;
        BEAST_EXPECT(router.shouldRecover(key1));
        BEAST_EXPECT(!router.shouldRecover(key1));
    }

    void
    testProcess()
    {
        using namespace std::chrono_literals;
        TestStopwatch stopwatch;
        HashRouter router(stopwatch, 5s, 5);
        uint256 const key(1);
        HashRouter::PeerShortID peer = 1;
        int flags;

        BEAST_EXPECT(router.shouldProcess(key, peer, flags, 1s));
        BEAST_EXPECT(! router.shouldProcess(key, peer, flags, 1s));
        ++stopwatch;
        ++stopwatch;
        BEAST_EXPECT(router.shouldProcess(key, peer, flags, 1s));
    }


public:

    void
    run() override
    {
        testNonExpiration();
        testExpiration();
        testSuppression();
        testSetFlags();
        testRelay();
        testRecover();
        testProcess();
    }
};

BEAST_DEFINE_TESTSUITE(HashRouter, app, ripple);

}
}
























