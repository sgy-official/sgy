

#include <test/nodestore/TestBase.h>
#include <ripple/nodestore/DummyScheduler.h>
#include <ripple/nodestore/Manager.h>
#include <ripple/beast/utility/temp_dir.h>
#include <test/unit_test/SuiteJournal.h>

namespace ripple {
namespace NodeStore {

class Database_test : public TestBase
{
    test::SuiteJournal journal_;

public:
    Database_test ()
    : journal_ ("Database_test", *this)
    { }

    void testImport (std::string const& destBackendType,
        std::string const& srcBackendType, std::int64_t seedValue)
    {
        DummyScheduler scheduler;
        RootStoppable parent ("TestRootStoppable");

        beast::temp_dir node_db;
        Section srcParams;
        srcParams.set ("type", srcBackendType);
        srcParams.set ("path", node_db.path());

        auto batch = createPredictableBatch (
            numObjectsToTest, seedValue);

        {
            std::unique_ptr <Database> src = Manager::instance().make_Database (
                "test", scheduler, 2, parent, srcParams, journal_);
            storeBatch (*src, batch);
        }

        Batch copy;

        {
            std::unique_ptr <Database> src = Manager::instance().make_Database (
                "test", scheduler, 2, parent, srcParams, journal_);

            beast::temp_dir dest_db;
            Section destParams;
            destParams.set ("type", destBackendType);
            destParams.set ("path", dest_db.path());

            std::unique_ptr <Database> dest = Manager::instance().make_Database (
                "test", scheduler, 2, parent, destParams, journal_);

            testcase ("import into '" + destBackendType +
                "' from '" + srcBackendType + "'");

            dest->import (*src);

            fetchCopyOfBatch (*dest, &copy, batch);
        }

        std::sort (batch.begin (), batch.end (), LessThan{});
        std::sort (copy.begin (), copy.end (), LessThan{});
        BEAST_EXPECT(areBatchesEqual (batch, copy));
    }


    void testNodeStore (std::string const& type,
                        bool const testPersistence,
                        std::int64_t const seedValue,
                        int numObjectsToTest = 2000)
    {
        DummyScheduler scheduler;
        RootStoppable parent ("TestRootStoppable");

        std::string s = "NodeStore backend '" + type + "'";

        testcase (s);

        beast::temp_dir node_db;
        Section nodeParams;
        nodeParams.set ("type", type);
        nodeParams.set ("path", node_db.path());

        beast::xor_shift_engine rng (seedValue);

        auto batch = createPredictableBatch (
            numObjectsToTest, rng());

        {
            std::unique_ptr <Database> db = Manager::instance().make_Database (
                "test", scheduler, 2, parent, nodeParams, journal_);

            storeBatch (*db, batch);

            {
                Batch copy;
                fetchCopyOfBatch (*db, &copy, batch);
                BEAST_EXPECT(areBatchesEqual (batch, copy));
            }

            {
                std::shuffle (
                    batch.begin(),
                    batch.end(),
                    rng);
                Batch copy;
                fetchCopyOfBatch (*db, &copy, batch);
                BEAST_EXPECT(areBatchesEqual (batch, copy));
            }
        }

        if (testPersistence)
        {
            std::unique_ptr <Database> db = Manager::instance().make_Database (
                "test", scheduler, 2, parent, nodeParams, journal_);

            Batch copy;
            fetchCopyOfBatch (*db, &copy, batch);

            std::sort (batch.begin (), batch.end (), LessThan{});
            std::sort (copy.begin (), copy.end (), LessThan{});
            BEAST_EXPECT(areBatchesEqual (batch, copy));
        }

        if (type == "memory")
        {
            {
                std::unique_ptr<Database> db =
                    Manager::instance().make_Database(
                        "test", scheduler, 2, parent, nodeParams, journal_);
                BEAST_EXPECT(db->earliestSeq() == XRP_LEDGER_EARLIEST_SEQ);
            }

            try
            {
                nodeParams.set("earliest_seq", "0");
                std::unique_ptr<Database> db =
                    Manager::instance().make_Database(
                        "test", scheduler, 2, parent, nodeParams, journal_);
            }
            catch (std::runtime_error const& e)
            {
                BEAST_EXPECT(std::strcmp(e.what(),
                    "Invalid earliest_seq") == 0);
            }

            {
                nodeParams.set("earliest_seq", "1");
                std::unique_ptr<Database> db =
                    Manager::instance().make_Database(
                        "test", scheduler, 2, parent, nodeParams, journal_);

                BEAST_EXPECT(db->earliestSeq() == 1);
            }


            try
            {
                nodeParams.set("earliest_seq",
                    std::to_string(XRP_LEDGER_EARLIEST_SEQ));
                std::unique_ptr<Database> db2 =
                    Manager::instance().make_Database(
                        "test", scheduler, 2, parent, nodeParams, journal_);
            }
            catch (std::runtime_error const& e)
            {
                BEAST_EXPECT(std::strcmp(e.what(),
                    "earliest_seq set more than once") == 0);
            }
        }
    }


    void run () override
    {
        std::int64_t const seedValue = 50;

        testNodeStore ("memory", false, seedValue);

        {
            testNodeStore ("nudb", true, seedValue);

        #if RIPPLE_ROCKSDB_AVAILABLE
            testNodeStore ("rocksdb", true, seedValue);
        #endif
        }

        {
            testImport ("nudb", "nudb", seedValue);

        #if RIPPLE_ROCKSDB_AVAILABLE
            testImport ("rocksdb", "rocksdb", seedValue);
        #endif

        #if RIPPLE_ENABLE_SQLITE_BACKEND_TESTS
            testImport ("sqlite", "sqlite", seedValue);
        #endif
        }
    }
};

BEAST_DEFINE_TESTSUITE(Database,NodeStore,ripple);

}
}
























