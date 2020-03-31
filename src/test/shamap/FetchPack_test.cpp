

#include <ripple/shamap/SHAMap.h>
#include <ripple/protocol/digest.h>
#include <ripple/basics/contract.h>
#include <ripple/basics/random.h>
#include <ripple/basics/StringUtilities.h>
#include <ripple/basics/UnorderedContainers.h>
#include <ripple/beast/xor_shift_engine.h>
#include <ripple/beast/unit_test.h>
#include <test/shamap/common.h>
#include <test/unit_test/SuiteJournal.h>
#include <functional>
#include <stdexcept>

namespace ripple {
namespace tests {

class FetchPack_test : public beast::unit_test::suite
{
public:
    enum
    {
        tableItems = 100,
        tableItemsExtra = 20
    };

    using Map   = hash_map <SHAMapHash, Blob>;
    using Table = SHAMap;
    using Item  = SHAMapItem;

    struct Handler
    {
        void operator()(std::uint32_t refNum) const
        {
            Throw<std::runtime_error> ("missing node");
        }
    };

    struct TestFilter : SHAMapSyncFilter
    {
        TestFilter (Map& map, beast::Journal journal) : mMap (map), mJournal (journal)
        {
        }

        void
        gotNode(bool fromFilter, SHAMapHash const& nodeHash,
            std::uint32_t ledgerSeq, Blob&& nodeData,
                SHAMapTreeNode::TNType type) const override
        {
        }

        boost::optional<Blob>
        getNode (SHAMapHash const& nodeHash) const override
        {
            Map::iterator it = mMap.find (nodeHash);
            if (it == mMap.end ())
            {
                JLOG(mJournal.fatal()) << "Test filter missing node";
                return boost::none;
            }
            return it->second;
        }

        Map& mMap;
        beast::Journal mJournal;
    };

    std::shared_ptr <Item>
    make_random_item (beast::xor_shift_engine& r)
    {
        Serializer s;
        for (int d = 0; d < 3; ++d)
            s.add32 (ripple::rand_int<std::uint32_t>(r));
        return std::make_shared <Item> (
            s.getSHA512Half(), s.peekData ());
    }

    void
    add_random_items (
        std::size_t n,
        Table& t,
        beast::xor_shift_engine& r)
    {
        while (n--)
        {
            std::shared_ptr <SHAMapItem> item (
                make_random_item (r));
            auto const result (t.addItem (std::move(*item), false, false));
            assert (result);
            (void) result;
        }
    }

    void on_fetch (Map& map, SHAMapHash const& hash, Blob const& blob)
    {
        BEAST_EXPECT(sha512Half(makeSlice(blob)) == hash.as_uint256());
        map.emplace (hash, blob);
    }

    void run () override
    {
        using namespace beast::severities;
        test::SuiteJournal journal ("FetchPack_test", *this);

        TestFamily f(journal);
        std::shared_ptr <Table> t1 (std::make_shared <Table> (
            SHAMapType::FREE, f, SHAMap::version{2}));

        pass ();



    }
};

BEAST_DEFINE_TESTSUITE(FetchPack,shamap,ripple);

} 
} 

























