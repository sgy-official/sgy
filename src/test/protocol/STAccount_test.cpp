

#include <ripple/protocol/STAccount.h>
#include <ripple/beast/unit_test.h>

namespace ripple {

struct STAccount_test : public beast::unit_test::suite
{
    void
    testSTAccount()
    {
        {
            STAccount const defaultAcct;
            BEAST_EXPECT(defaultAcct.getSType() == STI_ACCOUNT);
            BEAST_EXPECT(defaultAcct.getText() == "");
            BEAST_EXPECT(defaultAcct.isDefault() == true);
            BEAST_EXPECT(defaultAcct.value() == AccountID {});
            {
#ifdef NDEBUG 
                Serializer s;
                defaultAcct.add (s); 
                BEAST_EXPECT(s.size() == 1);
                BEAST_EXPECT(s.getHex() == "00");
                SerialIter sit (s.slice ());
                STAccount const deserializedDefault (sit, sfAccount);
                BEAST_EXPECT(deserializedDefault.isEquivalent (defaultAcct));
#endif 
            }
            {
                Serializer s;
                s.addVL (nullptr, 0);
                SerialIter sit (s.slice ());
                STAccount const deserializedDefault (sit, sfAccount);
                BEAST_EXPECT(deserializedDefault.isEquivalent (defaultAcct));
            }

            STAccount const sfAcct {sfAccount};
            BEAST_EXPECT(sfAcct.getSType() == STI_ACCOUNT);
            BEAST_EXPECT(sfAcct.getText() == "");
            BEAST_EXPECT(sfAcct.isDefault());
            BEAST_EXPECT(sfAcct.value() == AccountID {});
            BEAST_EXPECT(sfAcct.isEquivalent (defaultAcct));
            {
                Serializer s;
                sfAcct.add (s);
                BEAST_EXPECT(s.size() == 1);
                BEAST_EXPECT(s.getHex() == "00");
                SerialIter sit (s.slice ());
                STAccount const deserializedSf (sit, sfAccount);
                BEAST_EXPECT(deserializedSf.isEquivalent(sfAcct));
            }

            STAccount const zeroAcct {sfAccount, AccountID{}};
            BEAST_EXPECT(zeroAcct.getText() == "rrrrrrrrrrrrrrrrrrrrrhoLvTp");
            BEAST_EXPECT(! zeroAcct.isDefault());
            BEAST_EXPECT(zeroAcct.value() == AccountID {0});
            BEAST_EXPECT(! zeroAcct.isEquivalent (defaultAcct));
            BEAST_EXPECT(! zeroAcct.isEquivalent (sfAcct));
            {
                Serializer s;
                zeroAcct.add (s);
                BEAST_EXPECT(s.size() == 21);
                BEAST_EXPECT(s.getHex() ==
                    "140000000000000000000000000000000000000000");
                SerialIter sit (s.slice ());
                STAccount const deserializedZero (sit, sfAccount);
                BEAST_EXPECT(deserializedZero.isEquivalent (zeroAcct));
            }
            {
                Serializer s;
                const std::uint8_t bits128[] {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
                s.addVL (bits128, sizeof (bits128));
                SerialIter sit (s.slice ());
                try
                {
                    STAccount const deserializedBadSize (sit, sfAccount);
                }
                catch (std::runtime_error const& ex)
                {
                    BEAST_EXPECT(ex.what() == std::string("Invalid STAccount size"));
                }

            }

            STAccount const regKey {sfRegularKey, AccountID{}};
            BEAST_EXPECT(regKey.isEquivalent (zeroAcct));

            STAccount assignAcct;
            BEAST_EXPECT(assignAcct.isEquivalent (defaultAcct));
            BEAST_EXPECT(assignAcct.isDefault());
            assignAcct = AccountID{};
            BEAST_EXPECT(! assignAcct.isEquivalent (defaultAcct));
            BEAST_EXPECT(assignAcct.isEquivalent (zeroAcct));
            BEAST_EXPECT(! assignAcct.isDefault());
        }
    }

    void
    run() override
    {
        testSTAccount();
    }
};

BEAST_DEFINE_TESTSUITE(STAccount,protocol,ripple);

}
























