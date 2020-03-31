

#include <ripple/app/tx/impl/OfferStream.h>
#include <ripple/beast/unit_test.h>

namespace ripple {

class OfferStream_test : public beast::unit_test::suite
{
public:
    void
    test()
    {
        pass();
    }

    void
    run() override
    {
        test();
    }
};

BEAST_DEFINE_TESTSUITE(OfferStream,tx,ripple);

}
























