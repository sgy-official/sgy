

#include <ripple/protocol/PublicKey.h>
#include <ripple/protocol/digest.h>
#include <ripple/protocol/impl/secp256k1.h>
#include <ripple/basics/contract.h>
#include <ripple/basics/strHex.h>
#include <boost/multiprecision/cpp_int.hpp>
#include <ed25519-donna/ed25519.h>
#include <type_traits>

namespace ripple {

std::ostream&
operator<<(std::ostream& os, PublicKey const& pk)
{
    os << strHex(pk);
    return os;
}

template<>
boost::optional<PublicKey>
parseBase58 (TokenType type, std::string const& s)
{
    auto const result = decodeBase58Token(s, type);
    auto const pks = makeSlice(result);
    if (!publicKeyType(pks))
        return boost::none;
    return PublicKey(pks);
}


static
boost::optional<Slice>
sigPart (Slice& buf)
{
    if (buf.size() < 3 || buf[0] != 0x02)
        return boost::none;
    auto const len = buf[1];
    buf += 2;
    if (len > buf.size() || len < 1 || len > 33)
        return boost::none;
    if ((buf[0] & 0x80) != 0)
        return boost::none;
    if (buf[0] == 0)
    {
        if (len == 1)
            return boost::none;
        if ((buf[1] & 0x80) == 0)
            return boost::none;
    }
    boost::optional<Slice> number = Slice(buf.data(), len);
    buf += len;
    return number;
}

static
std::string
sliceToHex (Slice const& slice)
{
    std::string s;
    if (slice[0] & 0x80)
    {
        s.reserve(2 * (slice.size() + 2));
        s = "0x00";
    }
    else
    {
        s.reserve(2 * (slice.size() + 1));
        s = "0x";
    }
    for(int i = 0; i < slice.size(); ++i)
    {
        s += "0123456789ABCDEF"[((slice[i]&0xf0)>>4)];
        s += "0123456789ABCDEF"[((slice[i]&0x0f)>>0)];
    }
    return s;
}


boost::optional<ECDSACanonicality>
ecdsaCanonicality (Slice const& sig)
{
    using uint264 = boost::multiprecision::number<
        boost::multiprecision::cpp_int_backend<
            264, 264, boost::multiprecision::signed_magnitude,
            boost::multiprecision::unchecked, void>>;

    static uint264 const G(
        "0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEBAAEDCE6AF48A03BBFD25E8CD0364141");

    if ((sig.size() < 8) || (sig.size() > 72))
        return boost::none;
    if ((sig[0] != 0x30) || (sig[1] != (sig.size() - 2)))
        return boost::none;
    Slice p = sig + 2;
    auto r = sigPart(p);
    auto s = sigPart(p);
    if (! r || ! s || ! p.empty())
        return boost::none;

    uint264 R(sliceToHex(*r));
    if (R >= G)
        return boost::none;

    uint264 S(sliceToHex(*s));
    if (S >= G)
        return boost::none;

    auto const Sp = G - S;
    if (S > Sp)
        return ECDSACanonicality::canonical;
    return ECDSACanonicality::fullyCanonical;
}

static
bool
ed25519Canonical (Slice const& sig)
{
    if (sig.size() != 64)
        return false;
    std::uint8_t const Order[] =
    {
        0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x14, 0xDE, 0xF9, 0xDE, 0xA2, 0xF7, 0x9C, 0xD6,
        0x58, 0x12, 0x63, 0x1A, 0x5C, 0xF5, 0xD3, 0xED,
    };
    auto const le = sig.data() + 32;
    std::uint8_t S[32];
    std::reverse_copy(le, le + 32, S);
    return std::lexicographical_compare(
        S, S + 32, Order, Order + 32);
}


PublicKey::PublicKey (Slice const& slice)
{
    if(! publicKeyType(slice))
        LogicError("PublicKey::PublicKey invalid type");
    size_ = slice.size();
    std::memcpy(buf_, slice.data(), size_);
}

PublicKey::PublicKey (PublicKey const& other)
    : size_ (other.size_)
{
    if (size_)
        std::memcpy(buf_, other.buf_, size_);
};

PublicKey&
PublicKey::operator=(PublicKey const& other)
{
    size_ = other.size_;
    if (size_)
        std::memcpy(buf_, other.buf_, size_);
    return *this;
}


boost::optional<KeyType>
publicKeyType (Slice const& slice)
{
    if (slice.size() == 33)
    {
        if (slice[0] == 0xED)
            return KeyType::ed25519;

        if (slice[0] == 0x02 || slice[0] == 0x03)
            return KeyType::secp256k1;
    }

    return boost::none;
}

bool
verifyDigest (PublicKey const& publicKey,
    uint256 const& digest,
    Slice const& sig,
    bool mustBeFullyCanonical)
{
    if (publicKeyType(publicKey) != KeyType::secp256k1)
        LogicError("sign: secp256k1 required for digest signing");
    auto const canonicality = ecdsaCanonicality(sig);
    if (! canonicality)
        return false;
    if (mustBeFullyCanonical &&
        (*canonicality != ECDSACanonicality::fullyCanonical))
        return false;

    secp256k1_pubkey pubkey_imp;
    if(secp256k1_ec_pubkey_parse(
            secp256k1Context(),
            &pubkey_imp,
            reinterpret_cast<unsigned char const*>(
                publicKey.data()),
            publicKey.size()) != 1)
        return false;

    secp256k1_ecdsa_signature sig_imp;
    if(secp256k1_ecdsa_signature_parse_der(
            secp256k1Context(),
            &sig_imp,
            reinterpret_cast<unsigned char const*>(
                sig.data()),
            sig.size()) != 1)
        return false;
    if (*canonicality != ECDSACanonicality::fullyCanonical)
    {
        secp256k1_ecdsa_signature sig_norm;
        if(secp256k1_ecdsa_signature_normalize(
                secp256k1Context(),
                &sig_norm,
                &sig_imp) != 1)
            return false;
        return secp256k1_ecdsa_verify(
            secp256k1Context(),
            &sig_norm,
            reinterpret_cast<unsigned char const*>(
                digest.data()),
            &pubkey_imp) == 1;
    }
    return secp256k1_ecdsa_verify(
        secp256k1Context(),
        &sig_imp,
        reinterpret_cast<unsigned char const*>(
            digest.data()),
        &pubkey_imp) == 1;
}

bool
verify (PublicKey const& publicKey,
    Slice const& m,
    Slice const& sig,
    bool mustBeFullyCanonical)
{
    if (auto const type = publicKeyType(publicKey))
    {
        if (*type == KeyType::secp256k1)
        {
            return verifyDigest (publicKey,
                sha512Half(m), sig, mustBeFullyCanonical);
        }
        else if (*type == KeyType::ed25519)
        {
            if (! ed25519Canonical(sig))
                return false;

            return ed25519_sign_open(
                m.data(), m.size(), publicKey.data() + 1,
                    sig.data()) == 0;
        }
    }
    return false;
}

NodeID
calcNodeID (PublicKey const& pk)
{
    ripesha_hasher h;
    h(pk.data(), pk.size());
    auto const digest = static_cast<
        ripesha_hasher::result_type>(h);
    static_assert(NodeID::bytes ==
        sizeof(ripesha_hasher::result_type), "");
    NodeID result;
    std::memcpy(result.data(),
        digest.data(), digest.size());
    return result;
}

} 

























