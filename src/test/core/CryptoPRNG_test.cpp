

#include <test/jtx/Env.h>
#include <ripple/beast/utility/temp_dir.h>
#include <ripple/crypto/csprng.h>
#include <boost/filesystem.hpp>
#include <fstream>
#include <streambuf>

namespace ripple {

class CryptoPRNG_test : public beast::unit_test::suite
{
    void
    testGetValues()
    {
        testcase ("Get Values");
        try
        {
            auto& engine = crypto_prng();
            auto rand_val = engine();
            BEAST_EXPECT(rand_val >= engine.min());
            BEAST_EXPECT(rand_val <= engine.max());

            uint16_t twoByte {0};
            engine(&twoByte, sizeof(uint16_t));
            pass();
        }
        catch(std::exception&)
        {
            fail();
        }
    }

public:
    void run () override
    {
        testGetValues();
    }
};

BEAST_DEFINE_TESTSUITE (CryptoPRNG, core, ripple);

}  
























