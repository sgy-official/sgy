

#include <ripple/protocol/TER.h>
#include <ripple/beast/unit_test.h>

#include <tuple>
#include <type_traits>

namespace ripple {

struct TER_test : public beast::unit_test::suite
{
    void
    testTransResultInfo()
    {
        for (auto i = -400; i < 400; ++i)
        {
            TER t = TER::fromInt (i);
            auto inRange = isTelLocal(t) ||
                isTemMalformed(t) ||
                isTefFailure(t) ||
                isTerRetry(t) ||
                isTesSuccess(t) ||
                isTecClaim(t);

            std::string token, text;
            auto good = transResultInfo(t, token, text);
            BEAST_EXPECT(inRange || !good);
            BEAST_EXPECT(transToken(t) == (good ? token : "-"));
            BEAST_EXPECT(transHuman(t) == (good ? text : "-"));

            auto code = transCode(token);
            BEAST_EXPECT(good == !!code);
            BEAST_EXPECT(!code || *code == t);
        }
    }

    template<std::size_t I1, std::size_t I2>
    class NotConvertible
    {
    public:
        template<typename Tup>
        void operator()(Tup const& tup, beast::unit_test::suite&) const
        {
            using To_t = std::decay_t<decltype (std::get<I1>(tup))>;
            using From_t = std::decay_t<decltype (std::get<I2>(tup))>;
            static_assert (std::is_same<From_t, To_t>::value ==
                std::is_convertible<From_t, To_t>::value, "Convert err");
            static_assert (std::is_same<To_t, From_t>::value ==
                std::is_constructible<To_t, From_t>::value, "Construct err");
            static_assert (std::is_same <To_t, From_t>::value ==
                std::is_assignable<To_t&, From_t const&>::value, "Assign err");

            static_assert (
                ! std::is_convertible<int, To_t>::value, "Convert err");
            static_assert (
                ! std::is_constructible<To_t, int>::value, "Construct err");
            static_assert (
                ! std::is_assignable<To_t&, int const&>::value, "Assign err");
        }
    };

    template<std::size_t I1, std::size_t I2,
        template<std::size_t, std::size_t> class Func, typename Tup>
    std::enable_if_t<I1 != 0>
    testIterate (Tup const& tup, beast::unit_test::suite& suite)
    {
        Func<I1, I2> func;
        func (tup, suite);
        testIterate<I1 - 1, I2, Func>(tup, suite);
    }

    template<std::size_t I1, std::size_t I2,
        template<std::size_t, std::size_t> class Func, typename Tup>
    std::enable_if_t<I1 == 0 && I2 != 0>
    testIterate (Tup const& tup, beast::unit_test::suite& suite)
    {
        Func<I1, I2> func;
        func (tup, suite);
        testIterate<std::tuple_size<Tup>::value - 1, I2 - 1, Func>(tup, suite);
    }

    template<std::size_t I1, std::size_t I2,
        template<std::size_t, std::size_t> class Func, typename Tup>
    std::enable_if_t<I1 == 0 && I2 == 0>
    testIterate (Tup const& tup, beast::unit_test::suite& suite)
    {
        Func<I1, I2> func;
        func (tup, suite);
    }

    void testConversion()
    {

        static auto const terEnums = std::make_tuple (telLOCAL_ERROR,
            temMALFORMED, tefFAILURE, terRETRY, tesSUCCESS, tecCLAIM);
        static const int hiIndex {
            std::tuple_size<decltype (terEnums)>::value - 1};

        testIterate<hiIndex, hiIndex, NotConvertible> (terEnums, *this);

        auto isConvertable = [] (auto from, auto to)
        {
            using From_t = std::decay_t<decltype (from)>;
            using To_t = std::decay_t<decltype (to)>;
            static_assert (
                std::is_convertible<From_t, To_t>::value, "Convert err");
            static_assert (
                std::is_constructible<To_t, From_t>::value, "Construct err");
            static_assert (
                std::is_assignable<To_t&, From_t const&>::value, "Assign err");
        };

        NotTEC const notTec;
        isConvertable (telLOCAL_ERROR, notTec);
        isConvertable (temMALFORMED,   notTec);
        isConvertable (tefFAILURE,     notTec);
        isConvertable (terRETRY,       notTec);
        isConvertable (tesSUCCESS,     notTec);
        isConvertable (notTec,         notTec);

        auto notConvertible = [] (auto from, auto to)
        {
            using To_t = std::decay_t<decltype (to)>;
            using From_t = std::decay_t<decltype (from)>;
            static_assert (
                !std::is_convertible<From_t, To_t>::value, "Convert err");
            static_assert (
                !std::is_constructible<To_t, From_t>::value, "Construct err");
            static_assert (
                !std::is_assignable<To_t&, From_t const&>::value, "Assign err");
        };

        TER const ter;
        notConvertible (tecCLAIM, notTec);
        notConvertible (ter,      notTec);
        notConvertible (4,        notTec);

        isConvertable (telLOCAL_ERROR, ter);
        isConvertable (temMALFORMED,   ter);
        isConvertable (tefFAILURE,     ter);
        isConvertable (terRETRY,       ter);
        isConvertable (tesSUCCESS,     ter);
        isConvertable (tecCLAIM,       ter);
        isConvertable (notTec,         ter);
        isConvertable (ter,            ter);

        notConvertible (4, ter);
    }

    template<std::size_t I1, std::size_t I2>
    class CheckComparable
    {
    public:
        template<typename Tup>
        void operator()(Tup const& tup, beast::unit_test::suite& suite) const
        {
            auto const lhs = std::get<I1>(tup);
            auto const rhs = std::get<I2>(tup);

            static_assert (std::is_same<
                decltype (operator== (lhs, rhs)), bool>::value, "== err");

            static_assert (std::is_same<
                decltype (operator!= (lhs, rhs)), bool>::value, "!= err");

            static_assert (std::is_same<
                decltype (operator<  (lhs, rhs)), bool>::value, "< err");

            static_assert (std::is_same<
                decltype (operator<= (lhs, rhs)), bool>::value, "<= err");

            static_assert (std::is_same<
                decltype (operator>  (lhs, rhs)), bool>::value, "> err");

            static_assert (std::is_same<
                decltype (operator>= (lhs, rhs)), bool>::value, ">= err");

            suite.expect ((lhs == rhs) == (TERtoInt (lhs) == TERtoInt (rhs)));
            suite.expect ((lhs != rhs) == (TERtoInt (lhs) != TERtoInt (rhs)));
            suite.expect ((lhs <  rhs) == (TERtoInt (lhs) <  TERtoInt (rhs)));
            suite.expect ((lhs <= rhs) == (TERtoInt (lhs) <= TERtoInt (rhs)));
            suite.expect ((lhs >  rhs) == (TERtoInt (lhs) >  TERtoInt (rhs)));
            suite.expect ((lhs >= rhs) == (TERtoInt (lhs) >= TERtoInt (rhs)));
        }
    };

    void testComparison()
    {

        static auto const ters = std::make_tuple (telLOCAL_ERROR, temMALFORMED,
            tefFAILURE, terRETRY, tesSUCCESS, tecCLAIM,
            NotTEC {telLOCAL_ERROR}, TER {tecCLAIM});
        static const int hiIndex {
            std::tuple_size<decltype (ters)>::value - 1};

        testIterate<hiIndex, hiIndex, CheckComparable> (ters, *this);
    }

    void
    run() override
    {
        testTransResultInfo();
        testConversion();
        testComparison();
    }
};

BEAST_DEFINE_TESTSUITE(TER,protocol,ripple);

} 
























