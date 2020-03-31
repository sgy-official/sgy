

#ifndef RIPPLE_APP_MISC_MANIFEST_H_INCLUDED
#define RIPPLE_APP_MISC_MANIFEST_H_INCLUDED

#include <ripple/basics/UnorderedContainers.h>
#include <ripple/protocol/PublicKey.h>
#include <ripple/protocol/SecretKey.h>
#include <ripple/beast/utility/Journal.h>
#include <boost/optional.hpp>
#include <string>

namespace ripple {




struct Manifest
{
    std::string serialized;

    PublicKey masterKey;

    PublicKey signingKey;

    std::uint32_t sequence = 0;

    std::string domain;

    Manifest() = default;
    Manifest(Manifest const& other) = delete;
    Manifest& operator=(Manifest const& other) = delete;
    Manifest(Manifest&& other) = default;
    Manifest& operator=(Manifest&& other) = default;

    bool verify () const;

    uint256 hash () const;

    bool revoked () const;

    boost::optional<Blob> getSignature () const;

    Blob getMasterSignature () const;
};



boost::optional<Manifest>
deserializeManifest(Slice s);

inline
boost::optional<Manifest>
deserializeManifest(std::string const& s)
{
    return deserializeManifest(makeSlice(s));
}

template <class T, class = std::enable_if_t<
    std::is_same<T, char>::value || std::is_same<T, unsigned char>::value>>
boost::optional<Manifest>
deserializeManifest(std::vector<T> const& v)
{
    return deserializeManifest(makeSlice(v));
}


inline
bool
operator==(Manifest const& lhs, Manifest const& rhs)
{
    return lhs.sequence == rhs.sequence &&
        lhs.masterKey == rhs.masterKey &&
        lhs.signingKey == rhs.signingKey &&
        lhs.domain == rhs.domain &&
        lhs.serialized == rhs.serialized;
}

inline
bool
operator!=(Manifest const& lhs, Manifest const& rhs)
{
    return !(lhs == rhs);
}

struct ValidatorToken
{
    std::string manifest;
    SecretKey validationSecret;

private:
    ValidatorToken(std::string const& m, SecretKey const& valSecret);

public:
    ValidatorToken(ValidatorToken const&) = delete;
    ValidatorToken(ValidatorToken&& other) = default;

    static boost::optional<ValidatorToken>
    make_ValidatorToken(std::vector<std::string> const& tokenBlob);
};

enum class ManifestDisposition
{
    accepted = 0,

    stale,

    invalid
};

inline std::string
to_string(ManifestDisposition m)
{
    switch (m)
    {
        case ManifestDisposition::accepted:
            return "accepted";
        case ManifestDisposition::stale:
            return "stale";
        case ManifestDisposition::invalid:
            return "invalid";
        default:
            return "unknown";
    }
}

class DatabaseCon;


class ManifestCache
{
private:
    beast::Journal mutable j_;
    std::mutex apply_mutex_;
    std::mutex mutable read_mutex_;

    
    hash_map <PublicKey, Manifest> map_;

    
    hash_map <PublicKey, PublicKey> signingToMasterKeys_;

public:
    explicit
    ManifestCache (beast::Journal j =
        beast::Journal(beast::Journal::getNullSink()))
        : j_ (j)
    {
    }

    
    PublicKey
    getSigningKey (PublicKey const& pk) const;

    
    PublicKey
    getMasterKey (PublicKey const& pk) const;

    
    bool
    revoked (PublicKey const& pk) const;

    
    ManifestDisposition
    applyManifest (
        Manifest m);

    
    bool load (
        DatabaseCon& dbCon, std::string const& dbTable,
        std::string const& configManifest,
        std::vector<std::string> const& configRevocation);

    
    void load (
        DatabaseCon& dbCon, std::string const& dbTable);

    
    void save (
        DatabaseCon& dbCon, std::string const& dbTable,
        std::function <bool (PublicKey const&)> isTrusted);

    
    template <class Function>
    void
    for_each_manifest(Function&& f) const
    {
        std::lock_guard<std::mutex> lock{read_mutex_};
        for (auto const& m : map_)
        {
            f(m.second);
        }
    }

    
    template <class PreFun, class EachFun>
    void
    for_each_manifest(PreFun&& pf, EachFun&& f) const
    {
        std::lock_guard<std::mutex> lock{read_mutex_};
        pf(map_.size ());
        for (auto const& m : map_)
        {
            f(m.second);
        }
    }
};

} 

#endif








