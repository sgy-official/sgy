
#include <ripple/beast/unit_test.h>

namespace beast {

class Debug_test : public unit_test::suite
{
public:
    static int envDebug()
    {
#ifdef _DEBUG
        return 1;
#else
        return 0;
#endif
    }

    static int beastDebug()
    {
#ifdef BEAST_DEBUG
        return BEAST_DEBUG;
#else
        return 0;
#endif
    }

    void run() override
    {
        log <<
            "_DEBUG              = " << envDebug() << '\n' <<
            "BEAST_DEBUG         = " << beastDebug() << '\n' <<
            "sizeof(std::size_t) = " << sizeof(std::size_t) << std::endl;
        pass();
    }
};

BEAST_DEFINE_TESTSUITE(Debug, utility, beast);

}
























