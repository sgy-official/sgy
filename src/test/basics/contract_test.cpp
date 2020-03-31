

#include <ripple/basics/contract.h>
#include <ripple/beast/unit_test.h>
#include <string>

namespace ripple {

class contract_test : public beast::unit_test::suite
{
public:
    void run () override
    {
        try
        {
            Throw<std::runtime_error>("Throw test");
        }
        catch (std::runtime_error const& e)
        {
            BEAST_EXPECT(std::string(e.what()) == "Throw test");

            try
            {
                Rethrow();
            }
            catch (std::runtime_error const& e)
            {
                BEAST_EXPECT(std::string(e.what()) == "Throw test");
            }
            catch (...)
            {
                BEAST_EXPECT(false);
            }
        }
        catch (...)
        {
            BEAST_EXPECT(false);
        }
    }
};

BEAST_DEFINE_TESTSUITE(contract,basics,ripple);

}
























