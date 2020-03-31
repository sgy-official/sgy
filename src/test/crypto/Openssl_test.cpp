

#include <ripple/beast/unit_test.h>
#include <ripple/crypto/impl/openssl.h>
#include <type_traits>

namespace ripple {
struct Openssl_test : public beast::unit_test::suite
{
    void
    testBasicProperties()
    {
        using namespace openssl;

        BEAST_EXPECT(std::is_default_constructible<bignum>{});
        BEAST_EXPECT(!std::is_copy_constructible<bignum>{});
        BEAST_EXPECT(!std::is_copy_assignable<bignum>{});
        BEAST_EXPECT(std::is_nothrow_move_constructible<bignum>{});
        BEAST_EXPECT(std::is_nothrow_move_assignable<bignum>{});

        BEAST_EXPECT(!std::is_default_constructible<ec_point>{});
        BEAST_EXPECT(!std::is_copy_constructible<ec_point>{});
        BEAST_EXPECT(!std::is_copy_assignable<ec_point>{});
        BEAST_EXPECT(std::is_nothrow_move_constructible<ec_point>{});
        BEAST_EXPECT(!std::is_nothrow_move_assignable<ec_point>{});
    }

    void
    run() override
    {
        testBasicProperties();
    };
};

BEAST_DEFINE_TESTSUITE(Openssl, crypto, ripple);

}  
























