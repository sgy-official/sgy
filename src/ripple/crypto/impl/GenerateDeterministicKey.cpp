

#include <ripple/basics/contract.h>
#include <ripple/beast/crypto/secure_erase.h>
#include <ripple/crypto/GenerateDeterministicKey.h>
#include <ripple/crypto/impl/ec_key.h>
#include <ripple/crypto/impl/openssl.h>
#include <ripple/protocol/digest.h>
#include <array>
#include <string>
#include <openssl/pem.h>
#include <openssl/sha.h>

namespace ripple {

namespace openssl {

struct secp256k1_data
{
    EC_GROUP const* group;
    bignum order;

    secp256k1_data ()
    {
        group = EC_GROUP_new_by_curve_name (NID_secp256k1);

        if (!group)
            LogicError ("The OpenSSL library on this system lacks elliptic curve support.");

        bn_ctx ctx;
        order = get_order (group, ctx);
    }
};

static secp256k1_data const& secp256k1curve()
{
    static secp256k1_data const curve {};
    return curve;
}

}  

using namespace openssl;

static Blob serialize_ec_point (ec_point const& point)
{
    Blob result (33);

    serialize_ec_point (point, &result[0]);

    return result;
}

template <class FwdIt>
void
copy_uint32 (FwdIt out, std::uint32_t v)
{
    *out++ =  v >> 24;
    *out++ = (v >> 16) & 0xff;
    *out++ = (v >>  8) & 0xff;
    *out   =  v        & 0xff;
}


static bignum generateRootDeterministicKey (uint128 const& seed)
{
    bignum privKey;
    std::uint32_t seq = 0;

    do
    {
        std::array<std::uint8_t, 20> buf;
        std::copy(seed.begin(), seed.end(), buf.begin());
        copy_uint32 (buf.begin() + 16, seq++);
        auto root = sha512Half(buf);
        beast::secure_erase(buf.data(), buf.size());
        privKey.assign (root.data(), root.size());
        beast::secure_erase(root.data(), root.size());
    }
    while (privKey.is_zero() || privKey >= secp256k1curve().order);
    beast::secure_erase(&seq, sizeof(seq));
    return privKey;
}

Blob generateRootDeterministicPublicKey (uint128 const& seed)
{
    bn_ctx ctx;

    bignum privKey = generateRootDeterministicKey (seed);

    ec_point pubKey = multiply (secp256k1curve().group, privKey, ctx);

    privKey.clear();  

    return serialize_ec_point (pubKey);
}

uint256 generateRootDeterministicPrivateKey (uint128 const& seed)
{
    bignum key = generateRootDeterministicKey (seed);

    return uint256_from_bignum_clear (key);
}

static ec_point generateRootPubKey (bignum&& pubGenerator)
{
    ec_point pubPoint = bn2point (secp256k1curve().group, pubGenerator.get());

    return pubPoint;
}

static bignum makeHash (Blob const& pubGen, int seq, bignum const& order)
{
    int subSeq = 0;

    bignum result;

    assert(pubGen.size() == 33);
    do
    {
        std::array<std::uint8_t, 41> buf;
        std::copy (pubGen.begin(), pubGen.end(), buf.begin());
        copy_uint32 (buf.begin() + 33, seq);
        copy_uint32 (buf.begin() + 37, subSeq++);
        auto root = sha512Half_s(buf);
        beast::secure_erase(buf.data(), buf.size());
        result.assign (root.data(), root.size());
        beast::secure_erase(root.data(), root.size());
    }
    while (result.is_zero()  ||  result >= order);

    return result;
}

Blob generatePublicDeterministicKey (Blob const& pubGen, int seq)
{
    ec_point rootPubKey = generateRootPubKey (bignum (pubGen));

    bn_ctx ctx;

    bignum hash = makeHash (pubGen, seq, secp256k1curve().order);

    ec_point newPoint = multiply (secp256k1curve().group, hash, ctx);

    add_to (secp256k1curve().group, rootPubKey, newPoint, ctx);

    return serialize_ec_point (newPoint);
}

uint256 generatePrivateDeterministicKey (
    Blob const& pubGen, uint128 const& seed, int seq)
{
    bignum rootPrivKey = generateRootDeterministicKey (seed);

    bn_ctx ctx;

    bignum privKey = makeHash (pubGen, seq, secp256k1curve().order);

    add_to (rootPrivKey, privKey, secp256k1curve().order, ctx);

    rootPrivKey.clear();  

    return uint256_from_bignum_clear (privKey);
}

} 
























