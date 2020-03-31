

#include <ripple/basics/qalloc.h>
#include <ripple/beast/unit_test.h>
#include <type_traits>

namespace ripple {

struct qalloc_test : beast::unit_test::suite
{
    void
    testBasicProperties()
    {
        BEAST_EXPECT(std::is_default_constructible<qalloc>{});
        BEAST_EXPECT(std::is_copy_constructible<qalloc>{});
        BEAST_EXPECT(std::is_copy_assignable<qalloc>{});
        BEAST_EXPECT(std::is_nothrow_move_constructible<qalloc>{});
        BEAST_EXPECT(std::is_nothrow_move_assignable<qalloc>{});
    }

    void
    run() override
    {
        testBasicProperties();
    }
};

BEAST_DEFINE_TESTSUITE(qalloc, ripple_basics, ripple);

}  
























