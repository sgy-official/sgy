

#include <ripple/protocol/UintTypes.h>
#include <ripple/beast/unit_test.h>

namespace ripple {

struct types_test : public beast::unit_test::suite
{
    void
    testAccountID()
    {
        auto const s =
            "rHb9CJAWyB4rj91VRWn96DkukG4bwdtyTh";
        if (BEAST_EXPECT(parseBase58<AccountID>(s)))
            BEAST_EXPECT(toBase58(
                *parseBase58<AccountID>(s)) == s);
    }

    void
    run() override
    {
        testAccountID();
    }
};

BEAST_DEFINE_TESTSUITE(types,protocol,ripple);

}
























