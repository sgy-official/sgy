

#include <ripple/beast/unit_test.h>
#include <ripple/app/consensus/RCLCensorshipDetector.h>
#include <algorithm>
#include <vector>

namespace ripple {
namespace test {

class RCLCensorshipDetector_test : public beast::unit_test::suite
{
    void test(
        RCLCensorshipDetector<int, int>& cdet, int round,
        std::vector<int> proposed, std::vector<int> accepted,
        std::vector<int> remain, std::vector<int> remove)
    {
        RCLCensorshipDetector<int, int>::TxIDSeqVec proposal;
        for (auto const& i : proposed)
            proposal.emplace_back(i, round);
        cdet.propose(std::move(proposal));

        cdet.check(std::move(accepted),
            [&remove, &remain](auto id, auto seq)
            {
                if (std::find(remove.begin(), remove.end(), id) != remove.end())
                    return true;

                auto it = std::find(remain.begin(), remain.end(), id);
                if (it != remain.end())
                    remain.erase(it);
                return false;
            });

        BEAST_EXPECT(remain.empty());
    }

public:
    void
    run() override
    {
        testcase ("Censorship Detector");

        RCLCensorshipDetector<int, int> cdet;
        int round = 0;
        test(cdet, ++round,     { },                { },        { },            { });
        test(cdet, ++round,     { 10, 11, 12, 13 }, { 11, 2 },  { 10, 13 },     { });
        test(cdet, ++round,     { 10, 13, 14, 15 }, { 14 },     { 10, 13, 15 }, { });
        test(cdet, ++round,     { 10, 13, 15, 16 }, { 15, 16 }, { 10, 13 },     { });
        test(cdet, ++round,     { 10, 13 },         { 17, 18 }, { 10, 13 },     { });
        test(cdet, ++round,     { 10, 19 },         { },        { 10, 19 },     { });
        test(cdet, ++round,     { 10, 19, 20 },     { 20 },     { 10 },         { 19 });
        test(cdet, ++round,     { 21 },             { 21 },     { },            { });
        test(cdet, ++round,     { },                { 22 },     { },            { });
        test(cdet, ++round,     { 23, 24, 25, 26 }, { 25, 27 }, { 23, 26 },     { 24 });
        test(cdet, ++round,     { 23, 26, 28 },     { 26, 28 }, { 23 },         { });

        for (int i = 0; i != 10; ++i)
            test(cdet, ++round, { 23 },             { },        { 23 },         { });

        test(cdet, ++round,     { 23, 29 },         { 29 },     { 23 },         { });
        test(cdet, ++round,     { 30, 31 },         { 31 },     { 30 },         { });
        test(cdet, ++round,     { 30 },             { 30 },     { },            { });
        test(cdet, ++round,     { },                { },        { },            { });
    }
};

BEAST_DEFINE_TESTSUITE(RCLCensorshipDetector, app, ripple);
}  
}  
























