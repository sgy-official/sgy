

#include <ripple/basics/safe_cast.h>
#include <ripple/protocol/tokens.h>
#include <ripple/protocol/digest.h>
#include <boost/container/small_vector.hpp>
#include <cassert>
#include <cstring>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace ripple {

static char rippleAlphabet[] =
    "rpshnaf39wBUDNEGHJKLM4PQRST7VWXYZ2bcdeCg65jkm8oFqi1tuvAxyz";

static char bitcoinAlphabet[] =
    "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";


template <class Hasher>
static
typename Hasher::result_type
digest (void const* data, std::size_t size) noexcept
{
    Hasher h;
    h(data, size);
    return static_cast<
        typename Hasher::result_type>(h);
}

template <class Hasher, class T, std::size_t N,
    class = std::enable_if_t<sizeof(T) == 1>>
static
typename Hasher::result_type
digest (std::array<T, N> const& v)
{
    return digest<Hasher>(v.data(), v.size());
}

template <class Hasher, class... Args>
static
typename Hasher::result_type
digest2 (Args const&... args)
{
    return digest<Hasher>(
        digest<Hasher>(args...));
}


void
checksum (void* out,
    void const* message,
        std::size_t size)
{
    auto const h =
        digest2<sha256_hasher>(message, size);
    std::memcpy(out, h.data(), 4);
}


static
std::string
encodeBase58(
    void const* message, std::size_t size,
        void *temp, std::size_t temp_size,
            char const* const alphabet)
{
    auto pbegin = reinterpret_cast<unsigned char const*>(message);
    auto const pend = pbegin + size;

    int zeroes = 0;
    while (pbegin != pend && *pbegin == 0)
    {
        pbegin++;
        zeroes++;
    }

    auto const b58begin = reinterpret_cast<unsigned char*>(temp);
    auto const b58end = b58begin + temp_size;

    std::fill(b58begin, b58end, 0);

    while (pbegin != pend)
    {
        int carry = *pbegin;
        for (auto iter = b58end; iter != b58begin; --iter)
        {
            carry += 256 * (iter[-1]);
            iter[-1] = carry % 58;
            carry /= 58;
        }
        assert(carry == 0);
        pbegin++;
    }

    auto iter = b58begin;
    while (iter != b58end && *iter == 0)
        ++iter;

    std::string str;
    str.reserve(zeroes + (b58end - iter));
    str.assign(zeroes, alphabet[0]);
    while (iter != b58end)
        str += alphabet[*(iter++)];
    return str;
}

static
std::string
encodeToken (TokenType type,
    void const* token, std::size_t size, char const* const alphabet)
{
    auto const expanded = 1 + size + 4;

    auto const bufsize = expanded * 3;

    boost::container::small_vector<std::uint8_t, 1024> buf (bufsize);

    buf[0] = safe_cast<std::underlying_type_t <TokenType>>(type);
    if (size)
        std::memcpy(buf.data() + 1, token, size);
    checksum(buf.data() + 1 + size, buf.data(), 1 + size);

    return encodeBase58(
        buf.data(), expanded,
        buf.data() + expanded, bufsize - expanded,
        alphabet);
}

std::string
base58EncodeToken (TokenType type,
    void const* token, std::size_t size)
{
    return encodeToken(type, token, size, rippleAlphabet);
}

std::string
base58EncodeTokenBitcoin (TokenType type,
    void const* token, std::size_t size)
{
    return encodeToken(type, token, size, bitcoinAlphabet);
}


template <class InverseArray>
static
std::string
decodeBase58 (std::string const& s,
    InverseArray const& inv)
{
    auto psz = s.c_str();
    auto remain = s.size();
    int zeroes = 0;
    while (remain > 0 && inv[*psz] == 0)
    {
        ++zeroes;
        ++psz;
        --remain;
    }
    std::vector<unsigned char> b256(
        remain * 733 / 1000 + 1);
    while (remain > 0)
    {
        auto carry = inv[*psz];
        if (carry == -1)
            return {};
        for (auto iter = b256.rbegin();
            iter != b256.rend(); ++iter)
        {
            carry += 58 * *iter;
            *iter = carry % 256;
            carry /= 256;
        }
        assert(carry == 0);
        ++psz;
        --remain;
    }
    auto iter = std::find_if(
        b256.begin(), b256.end(),[](unsigned char c)
            { return c != 0; });
    std::string result;
    result.reserve (zeroes + (b256.end() - iter));
    result.assign (zeroes, 0x00);
    while (iter != b256.end())
        result.push_back(*(iter++));
    return result;
}


template <class InverseArray>
static
std::string
decodeBase58Token (std::string const& s,
    TokenType type, InverseArray const& inv)
{
    std::string const ret = decodeBase58(s, inv);

    if (ret.size() < 6)
        return {};

    if (type != safe_cast<TokenType>(static_cast<std::uint8_t>(ret[0])))
        return {};

    std::array<char, 4> guard;
    checksum(guard.data(), ret.data(), ret.size() - guard.size());
    if (!std::equal (guard.rbegin(), guard.rend(), ret.rbegin()))
        return {};

    return ret.substr(1, ret.size() - 1 - guard.size());
}


class InverseAlphabet
{
private:
    std::array<int, 256> map_;

public:
    explicit
    InverseAlphabet(std::string const& digits)
    {
        map_.fill(-1);
        int i = 0;
        for(auto const c : digits)
            map_[static_cast<
                unsigned char>(c)] = i++;
    }

    int
    operator[](char c) const
    {
        return map_[static_cast<
            unsigned char>(c)];
    }
};

static InverseAlphabet rippleInverse(rippleAlphabet);

static InverseAlphabet bitcoinInverse(bitcoinAlphabet);

std::string
decodeBase58Token(
    std::string const& s, TokenType type)
{
    return decodeBase58Token(s, type, rippleInverse);
}

std::string
decodeBase58TokenBitcoin(
    std::string const& s, TokenType type)
{
    return decodeBase58Token(s, type, bitcoinInverse);
}

} 
























