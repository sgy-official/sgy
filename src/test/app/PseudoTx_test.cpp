

#include <ripple/app/tx/apply.h>
#include <ripple/protocol/STAccount.h>
#include <string>
#include <test/jtx.h>
#include <vector>

namespace ripple {
namespace test {

struct PseudoTx_test : public beast::unit_test::suite
{
    std::vector<STTx>
    getPseudoTxs(std::uint32_t seq)
    {
        std::vector<STTx> res;

        res.emplace_back(STTx(ttFEE, [&](auto& obj) {
            obj[sfAccount] = AccountID();
            obj[sfLedgerSequence] = seq;
            obj[sfBaseFee] = 0;
            obj[sfReserveBase] = 0;
            obj[sfReserveIncrement] = 0;
            obj[sfReferenceFeeUnits] = 0;
        }));

        res.emplace_back(STTx(ttAMENDMENT, [&](auto& obj) {
            obj.setAccountID(sfAccount, AccountID());
            obj.setFieldH256(sfAmendment, uint256(2));
            obj.setFieldU32(sfLedgerSequence, seq);
        }));

        return res;
    }

    std::vector<STTx>
    getRealTxs()
    {
        std::vector<STTx> res;

        res.emplace_back(STTx(ttACCOUNT_SET, [&](auto& obj) {
            obj[sfAccount] = AccountID(1);
        }));

        res.emplace_back(STTx(ttPAYMENT, [&](auto& obj) {
            obj.setAccountID(sfAccount, AccountID(2));
            obj.setAccountID(sfDestination, AccountID(3));
        }));

        return res;
    }

    void
    testPrevented()
    {
        using namespace jtx;
        Env env(*this);

        for (auto const& stx : getPseudoTxs(env.closed()->seq() + 1))
        {
            std::string reason;
            BEAST_EXPECT(isPseudoTx(stx));
            BEAST_EXPECT(!passesLocalChecks(stx, reason));
            BEAST_EXPECT(reason == "Cannot submit pseudo transactions.");
            env.app().openLedger().modify(
                [&](OpenView& view, beast::Journal j) {
                    auto const result =
                        ripple::apply(env.app(), view, stx, tapNONE, j);
                    BEAST_EXPECT(!result.second && result.first == temINVALID);
                    return result.second;
                });
        }
    }

    void
    testAllowed()
    {
        for (auto const& stx : getRealTxs())
        {
            std::string reason;
            BEAST_EXPECT(!isPseudoTx(stx));
            BEAST_EXPECT(passesLocalChecks(stx, reason));
        }
    }

    void
    run() override
    {
        testPrevented();
        testAllowed();
    }
};

BEAST_DEFINE_TESTSUITE(PseudoTx, app, ripple);

}  
}  
























