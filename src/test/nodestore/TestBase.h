

#ifndef RIPPLE_NODESTORE_BASE_H_INCLUDED
#define RIPPLE_NODESTORE_BASE_H_INCLUDED

#include <ripple/nodestore/Database.h>
#include <ripple/basics/random.h>
#include <ripple/basics/StringUtilities.h>
#include <ripple/beast/unit_test.h>
#include <ripple/beast/utility/rngfill.h>
#include <ripple/beast/xor_shift_engine.h>
#include <ripple/nodestore/Backend.h>
#include <ripple/nodestore/Types.h>
#include <boost/algorithm/string.hpp>
#include <iomanip>

namespace ripple {
namespace NodeStore {


struct LessThan
{
    bool
    operator()(
        std::shared_ptr<NodeObject> const& lhs,
            std::shared_ptr<NodeObject> const& rhs) const noexcept
    {
        return lhs->getHash () < rhs->getHash ();
    }
};


inline
bool isSame (std::shared_ptr<NodeObject> const& lhs,
    std::shared_ptr<NodeObject> const& rhs)
{
    return
        (lhs->getType() == rhs->getType()) &&
        (lhs->getHash() == rhs->getHash()) &&
        (lhs->getData() == rhs->getData());
}

class TestBase : public beast::unit_test::suite
{
public:
    static std::size_t const minPayloadBytes = 1;
    static std::size_t const maxPayloadBytes = 2000;
    static int const numObjectsToTest = 2000;

public:
    static
    Batch createPredictableBatch(
        int numObjects, std::uint64_t seed)
    {
        Batch batch;
        batch.reserve (numObjects);

        beast::xor_shift_engine rng (seed);

        for (int i = 0; i < numObjects; ++i)
        {
            NodeObjectType type;

            switch (rand_int(rng, 3))
            {
            case 0: type = hotLEDGER; break;
            case 1: type = hotACCOUNT_NODE; break;
            case 2: type = hotTRANSACTION_NODE; break;
            case 3: type = hotUNKNOWN; break;
            }

            uint256 hash;
            beast::rngfill (hash.begin(), hash.size(), rng);

            Blob blob (
                rand_int(rng,
                    minPayloadBytes, maxPayloadBytes));
            beast::rngfill (blob.data(), blob.size(), rng);

            batch.push_back (
                NodeObject::createObject(
                    type, std::move(blob), hash));
        }

        return batch;
    }

    static bool areBatchesEqual (Batch const& lhs, Batch const& rhs)
    {
        bool result = true;

        if (lhs.size () == rhs.size ())
        {
            for (int i = 0; i < lhs.size (); ++i)
            {
                if (! isSame(lhs[i], rhs[i]))
                {
                    result = false;
                    break;
                }
            }
        }
        else
        {
            result = false;
        }

        return result;
    }

    void storeBatch (Backend& backend, Batch const& batch)
    {
        for (int i = 0; i < batch.size (); ++i)
        {
            backend.store (batch [i]);
        }
    }

    void fetchCopyOfBatch (Backend& backend, Batch* pCopy, Batch const& batch)
    {
        pCopy->clear ();
        pCopy->reserve (batch.size ());

        for (int i = 0; i < batch.size (); ++i)
        {
            std::shared_ptr<NodeObject> object;

            Status const status = backend.fetch (
                batch [i]->getHash ().cbegin (), &object);

            BEAST_EXPECT(status == ok);

            if (status == ok)
            {
                BEAST_EXPECT(object != nullptr);

                pCopy->push_back (object);
            }
        }
    }

    void fetchMissing(Backend& backend, Batch const& batch)
    {
        for (int i = 0; i < batch.size (); ++i)
        {
            std::shared_ptr<NodeObject> object;

            Status const status = backend.fetch (
                batch [i]->getHash ().cbegin (), &object);

            BEAST_EXPECT(status == notFound);
        }
    }

    static void storeBatch (Database& db, Batch const& batch)
    {
        for (int i = 0; i < batch.size (); ++i)
        {
            std::shared_ptr<NodeObject> const object (batch [i]);

            Blob data (object->getData ());

            db.store (object->getType (),
                      std::move (data),
                      object->getHash (),
                      db.earliestSeq());
        }
    }

    static void fetchCopyOfBatch (Database& db,
                                  Batch* pCopy,
                                  Batch const& batch)
    {
        pCopy->clear ();
        pCopy->reserve (batch.size ());

        for (int i = 0; i < batch.size (); ++i)
        {
            std::shared_ptr<NodeObject> object = db.fetch (
                batch [i]->getHash (), 0);

            if (object != nullptr)
                pCopy->push_back (object);
        }
    }
};

}
}

#endif








