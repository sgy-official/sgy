

#include <test/nodestore/TestBase.h>
#include <ripple/nodestore/DummyScheduler.h>
#include <ripple/nodestore/Manager.h>
#include <ripple/nodestore/impl/DecodedBlob.h>
#include <ripple/nodestore/impl/EncodedBlob.h>

namespace ripple {
namespace NodeStore {

class NodeStoreBasic_test : public TestBase
{
public:
    void testBatches (std::uint64_t const seedValue)
    {
        testcase ("batch");

        auto batch1 = createPredictableBatch (
            numObjectsToTest, seedValue);

        auto batch2 = createPredictableBatch (
            numObjectsToTest, seedValue);

        BEAST_EXPECT(areBatchesEqual (batch1, batch2));

        auto batch3 = createPredictableBatch (
            numObjectsToTest, seedValue + 1);

        BEAST_EXPECT(! areBatchesEqual (batch1, batch3));
    }

    void testBlobs (std::uint64_t const seedValue)
    {
        testcase ("encoding");

        auto batch = createPredictableBatch (
            numObjectsToTest, seedValue);

        EncodedBlob encoded;
        for (int i = 0; i < batch.size (); ++i)
        {
            encoded.prepare (batch [i]);

            DecodedBlob decoded (encoded.getKey (), encoded.getData (), encoded.getSize ());

            BEAST_EXPECT(decoded.wasOk ());

            if (decoded.wasOk ())
            {
                std::shared_ptr<NodeObject> const object (decoded.createObject ());

                BEAST_EXPECT(isSame(batch[i], object));
            }
        }
    }

    void run () override
    {
        std::uint64_t const seedValue = 50;

        testBatches (seedValue);

        testBlobs (seedValue);
    }
};

BEAST_DEFINE_TESTSUITE(NodeStoreBasic,ripple_core,ripple);

}
}
























