

#include <ripple/unity/rocksdb.h>
#include <ripple/nodestore/DummyScheduler.h>
#include <ripple/nodestore/Manager.h>
#include <ripple/beast/utility/temp_dir.h>
#include <test/nodestore/TestBase.h>
#include <test/unit_test/SuiteJournal.h>
#include <algorithm>

namespace ripple {
namespace NodeStore {

class Backend_test : public TestBase
{
public:
    void testBackend (
        std::string const& type,
        std::uint64_t const seedValue,
        int numObjectsToTest = 2000)
    {
        DummyScheduler scheduler;

        testcase ("Backend type=" + type);

        Section params;
        beast::temp_dir tempDir;
        params.set ("type", type);
        params.set ("path", tempDir.path());

        beast::xor_shift_engine rng (seedValue);

        auto batch = createPredictableBatch (
            numObjectsToTest, rng());

        using namespace beast::severities;
        test::SuiteJournal journal ("Backend_test", *this);

        {
            std::unique_ptr <Backend> backend =
                Manager::instance().make_Backend (
                    params, scheduler, journal);
            backend->open();

            storeBatch (*backend, batch);

            {
                Batch copy;
                fetchCopyOfBatch (*backend, &copy, batch);
                BEAST_EXPECT(areBatchesEqual (batch, copy));
            }

            {
                std::shuffle (
                    batch.begin(),
                    batch.end(),
                    rng);
                Batch copy;
                fetchCopyOfBatch (*backend, &copy, batch);
                BEAST_EXPECT(areBatchesEqual (batch, copy));
            }
        }

        {
            std::unique_ptr <Backend> backend = Manager::instance().make_Backend (
                params, scheduler, journal);
            backend->open();

            Batch copy;
            fetchCopyOfBatch (*backend, &copy, batch);
            std::sort (batch.begin (), batch.end (), LessThan{});
            std::sort (copy.begin (), copy.end (), LessThan{});
            BEAST_EXPECT(areBatchesEqual (batch, copy));
        }
    }


    void run () override
    {
        std::uint64_t const seedValue = 50;

        testBackend ("nudb", seedValue);

    #if RIPPLE_ROCKSDB_AVAILABLE
        testBackend ("rocksdb", seedValue);
    #endif

    #ifdef RIPPLE_ENABLE_SQLITE_BACKEND_TESTS
        testBackend ("sqlite", seedValue);
    #endif
    }
};

BEAST_DEFINE_TESTSUITE(Backend,ripple_core,ripple);

}
}
























