

#include <ripple/basics/base_uint.h>
#include <ripple/basics/Blob.h>
#include <ripple/basics/hardened_hash.h>
#include <ripple/beast/unit_test.h>
#include <boost/algorithm/string.hpp>
#include <complex>

#include <type_traits>

namespace ripple {
namespace test {

template <std::size_t Bits>
struct nonhash
{
    static constexpr std::size_t WIDTH = Bits / 8;
    std::array<std::uint8_t, WIDTH> data_;
    static beast::endian const endian = beast::endian::big;

    nonhash() = default;

    void
    operator() (void const* key, std::size_t len) noexcept
    {
        assert(len == WIDTH);
        memcpy(data_.data(), key, len);
    }

    explicit
    operator std::size_t() noexcept { return WIDTH; }
};

struct base_uint_test : beast::unit_test::suite
{
    using test96 = base_uint<96>;
    static_assert(std::is_copy_constructible<test96>::value, "");
    static_assert(std::is_copy_assignable<test96>::value, "");

    void run() override
    {
        static_assert(!std::is_constructible<test96, std::complex<double>>::value, "");
        static_assert(!std::is_assignable<test96&, std::complex<double>>::value, "");
        std::unordered_set<test96, hardened_hash<>> uset;

        Blob raw { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };
        BEAST_EXPECT(test96::bytes == raw.size());

        test96 u { raw };
        uset.insert(u);
        BEAST_EXPECT(raw.size() == u.size());
        BEAST_EXPECT(to_string(u) == "0102030405060708090A0B0C");
        BEAST_EXPECT(*u.data() == 1);
        BEAST_EXPECT(u.signum() == 1);
        BEAST_EXPECT(!!u);
        BEAST_EXPECT(!u.isZero());
        BEAST_EXPECT(u.isNonZero());
        unsigned char t = 0;
        for (auto& d : u)
        {
            BEAST_EXPECT(d == ++t);
        }

        nonhash<96> h;
        hash_append(h, u);
        test96 w {std::vector<std::uint8_t>(h.data_.begin(), h.data_.end())};
        BEAST_EXPECT(w == u);

        test96 v { ~u };
        uset.insert(v);
        BEAST_EXPECT(to_string(v) == "FEFDFCFBFAF9F8F7F6F5F4F3");
        BEAST_EXPECT(*v.data() == 0xfe);
        BEAST_EXPECT(v.signum() == 1);
        BEAST_EXPECT(!!v);
        BEAST_EXPECT(!v.isZero());
        BEAST_EXPECT(v.isNonZero());
        t = 0xff;
        for (auto& d : v)
        {
            BEAST_EXPECT(d == --t);
        }

        BEAST_EXPECT(compare(u, v) < 0);
        BEAST_EXPECT(compare(v, u) > 0);

        v = u;
        BEAST_EXPECT(v == u);

        test96 z { beast::zero };
        uset.insert(z);
        BEAST_EXPECT(to_string(z) == "000000000000000000000000");
        BEAST_EXPECT(*z.data() == 0);
        BEAST_EXPECT(*z.begin() == 0);
        BEAST_EXPECT(*std::prev(z.end(), 1) == 0);
        BEAST_EXPECT(z.signum() == 0);
        BEAST_EXPECT(!z);
        BEAST_EXPECT(z.isZero());
        BEAST_EXPECT(!z.isNonZero());
        for (auto& d : z)
        {
            BEAST_EXPECT(d == 0);
        }

        test96 n { z };
        n++;
        BEAST_EXPECT(n == test96(1));
        n--;
        BEAST_EXPECT(n == beast::zero);
        BEAST_EXPECT(n == z);
        n--;
        BEAST_EXPECT(to_string(n) == "FFFFFFFFFFFFFFFFFFFFFFFF");
        n = beast::zero;
        BEAST_EXPECT(n == z);

        test96 zp1 { z };
        zp1++;
        test96 zm1 { z };
        zm1--;
        test96 x { zm1 ^ zp1 };
        uset.insert(x);
        BEAST_EXPECTS(to_string(x) == "FFFFFFFFFFFFFFFFFFFFFFFE", to_string(x));

        BEAST_EXPECT(uset.size() == 4);

        test96 fromHex;
        BEAST_EXPECT(fromHex.SetHexExact(to_string(u)));
        BEAST_EXPECT(fromHex == u);
        fromHex = z;

        BEAST_EXPECT(! fromHex.SetHexExact("A" + to_string(u)));
        fromHex = z;

        BEAST_EXPECT(! fromHex.SetHexExact(to_string(u) + "A"));
        fromHex = z;

        BEAST_EXPECT(fromHex.SetHex(to_string(u)));
        BEAST_EXPECT(fromHex == u);
        fromHex = z;

        BEAST_EXPECT(fromHex.SetHex("  0x" + to_string(u)));
        BEAST_EXPECT(fromHex == u);
        fromHex = z;

        BEAST_EXPECT(fromHex.SetHex("FEFEFE" + to_string(u)));
        BEAST_EXPECT(fromHex == u);
        fromHex = z;

        BEAST_EXPECT(! fromHex.SetHex(
            boost::algorithm::replace_all_copy(to_string(u), "0", "Z")));
        fromHex = z;

        BEAST_EXPECT(fromHex.SetHex(to_string(u), true));
        BEAST_EXPECT(fromHex == u);
        fromHex = z;

        BEAST_EXPECT(! fromHex.SetHex("  0x" + to_string(u), true));
        fromHex = z;

        BEAST_EXPECT(fromHex.SetHex("DEAD" + to_string(u), true ));
        BEAST_EXPECT(fromHex == u);
        fromHex = z;

        BEAST_EXPECT(fromHex.SetHex("DEAD" + to_string(u), false ));
        BEAST_EXPECT(fromHex == u);
        fromHex = z;
    }
};

BEAST_DEFINE_TESTSUITE(base_uint, ripple_basics, ripple);

}  
}  
























