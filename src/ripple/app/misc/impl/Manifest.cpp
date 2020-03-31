

#include <ripple/app/misc/Manifest.h>
#include <ripple/basics/base64.h>
#include <ripple/basics/contract.h>
#include <ripple/basics/Log.h>
#include <ripple/basics/StringUtilities.h>
#include <ripple/beast/rfc2616.h>
#include <ripple/core/DatabaseCon.h>
#include <ripple/json/json_reader.h>
#include <ripple/protocol/PublicKey.h>
#include <ripple/protocol/Sign.h>
#include <boost/algorithm/clamp.hpp>
#include <boost/regex.hpp>
#include <numeric>
#include <stdexcept>

namespace ripple {

boost::optional<Manifest> deserializeManifest(Slice s)
{
    if (s.empty())
        return boost::none;

    static SOTemplate const manifestFormat {
            {sfPublicKey,       soeREQUIRED},

            {sfMasterSignature, soeREQUIRED},

            {sfSequence,        soeREQUIRED},

            {sfVersion,         soeDEFAULT},

            {sfDomain,          soeOPTIONAL},

            {sfSigningPubKey,   soeOPTIONAL},

            {sfSignature,       soeOPTIONAL},
        };

    try
    {
        SerialIter sit{ s };
        STObject st{ sit, sfGeneric };

        st.applyTemplate(manifestFormat);

        if (st.isFieldPresent(sfVersion) && st.getFieldU16(sfVersion) != 0)
            return boost::none;

        auto const pk = st.getFieldVL (sfPublicKey);

        if (! publicKeyType (makeSlice(pk)))
            return boost::none;

        Manifest m;
        m.serialized.assign(reinterpret_cast<char const*>(s.data()), s.size());
        m.masterKey = PublicKey(makeSlice(pk));
        m.sequence = st.getFieldU32 (sfSequence);

        if (st.isFieldPresent(sfDomain))
        {
            auto const d = st.getFieldVL(sfDomain);

            if (boost::algorithm::clamp(d.size(), 4, 128) != d.size())
                return boost::none;

            m.domain.assign (reinterpret_cast<char const*>(d.data()), d.size());

            static boost::regex const re(
                "^"                     
                "("                     
                "(?!-)"                 
                "[a-zA-Z0-9-]{1,63}"    
                "(?<!-)"                
                "\\."                   
                ")+"                    
                "[A-Za-z]{2,63}"        
                "$"                     
                , boost::regex_constants::optimize);

            if (!boost::regex_match(m.domain, re))
                return boost::none;
        }

        bool const hasEphemeralKey = st.isFieldPresent(sfSigningPubKey);
        bool const hasEphemeralSig = st.isFieldPresent(sfSignature);

        if (m.revoked())
        {
            if (hasEphemeralKey)
                return boost::none;

            if (hasEphemeralSig)
                return boost::none;
        }
        else
        {
            if (!hasEphemeralKey)
                return boost::none;

            if (!hasEphemeralSig)
                return boost::none;

            auto const spk = st.getFieldVL(sfSigningPubKey);

            if (!publicKeyType (makeSlice(spk)))
                return boost::none;

            m.signingKey = PublicKey(makeSlice(spk));
        }

        return std::move(m);
    }
    catch (std::exception const&)
    {
        return boost::none;
    }
}

template<class Stream>
Stream&
logMftAct (
    Stream& s,
    std::string const& action,
    PublicKey const& pk,
    std::uint32_t seq)
{
    s << "Manifest: " << action <<
         ";Pk: " << toBase58 (TokenType::NodePublic, pk) <<
         ";Seq: " << seq << ";";
    return s;
}

template<class Stream>
Stream& logMftAct (
    Stream& s,
    std::string const& action,
    PublicKey const& pk,
    std::uint32_t seq,
    std::uint32_t oldSeq)
{
    s << "Manifest: " << action <<
         ";Pk: " << toBase58 (TokenType::NodePublic, pk) <<
         ";Seq: " << seq <<
         ";OldSeq: " << oldSeq << ";";
    return s;
}

bool Manifest::verify () const
{
    STObject st (sfGeneric);
    SerialIter sit (serialized.data (), serialized.size ());
    st.set (sit);

    if (! revoked () && ! ripple::verify (st, HashPrefix::manifest, signingKey))
        return false;

    return ripple::verify (
        st, HashPrefix::manifest, masterKey, sfMasterSignature);
}

uint256 Manifest::hash () const
{
    STObject st (sfGeneric);
    SerialIter sit (serialized.data (), serialized.size ());
    st.set (sit);
    return st.getHash (HashPrefix::manifest);
}

bool Manifest::revoked () const
{
    
    return sequence == std::numeric_limits<std::uint32_t>::max ();
}

boost::optional<Blob> Manifest::getSignature () const
{
    STObject st (sfGeneric);
    SerialIter sit (serialized.data (), serialized.size ());
    st.set (sit);
    if (!get(st, sfSignature))
        return boost::none;
    return st.getFieldVL (sfSignature);
}

Blob Manifest::getMasterSignature () const
{
    STObject st (sfGeneric);
    SerialIter sit (serialized.data (), serialized.size ());
    st.set (sit);
    return st.getFieldVL (sfMasterSignature);
}

ValidatorToken::ValidatorToken(std::string const& m, SecretKey const& valSecret)
    : manifest(m)
    , validationSecret(valSecret)
{
}

boost::optional<ValidatorToken>
ValidatorToken::make_ValidatorToken(std::vector<std::string> const& tokenBlob)
{
    try
    {
        std::string tokenStr;
        tokenStr.reserve (
            std::accumulate (tokenBlob.cbegin(), tokenBlob.cend(), std::size_t(0),
                [] (std::size_t init, std::string const& s)
                {
                    return init + s.size();
                }));

        for (auto const& line : tokenBlob)
            tokenStr += beast::rfc2616::trim(line);

        tokenStr = base64_decode(tokenStr);

        Json::Reader r;
        Json::Value token;
        if (! r.parse (tokenStr, token))
            return boost::none;

        if (token.isMember("manifest") && token["manifest"].isString() &&
            token.isMember("validation_secret_key") &&
            token["validation_secret_key"].isString())
        {
            auto const ret = strUnHex (token["validation_secret_key"].asString());
            if (! ret.second || ret.first.empty())
                return boost::none;

            return ValidatorToken(
                token["manifest"].asString(),
                SecretKey(Slice{ret.first.data(), ret.first.size()}));
        }
        else
        {
            return boost::none;
        }
    }
    catch (std::exception const&)
    {
        return boost::none;
    }
}

PublicKey
ManifestCache::getSigningKey (PublicKey const& pk) const
{
    std::lock_guard<std::mutex> lock{read_mutex_};
    auto const iter = map_.find (pk);

    if (iter != map_.end () && !iter->second.revoked ())
        return iter->second.signingKey;

    return pk;
}

PublicKey
ManifestCache::getMasterKey (PublicKey const& pk) const
{
    std::lock_guard<std::mutex> lock{read_mutex_};
    auto const iter = signingToMasterKeys_.find (pk);

    if (iter != signingToMasterKeys_.end ())
        return iter->second;

    return pk;
}

bool
ManifestCache::revoked (PublicKey const& pk) const
{
    std::lock_guard<std::mutex> lock{read_mutex_};
    auto const iter = map_.find (pk);

    if (iter != map_.end ())
        return iter->second.revoked ();

    return false;
}

ManifestDisposition
ManifestCache::applyManifest (Manifest m)
{
    std::lock_guard<std::mutex> applyLock{apply_mutex_};

    
    auto const iter = map_.find(m.masterKey);

    if (iter != map_.end() &&
        m.sequence <= iter->second.sequence)
    {
        
        if (auto stream = j_.debug())
            logMftAct(stream, "Stale", m.masterKey, m.sequence, iter->second.sequence);
        return ManifestDisposition::stale;  
    }

    if (! m.verify())
    {
        
        if (auto stream = j_.warn())
            logMftAct(stream, "Invalid", m.masterKey, m.sequence);
        return ManifestDisposition::invalid;
    }

    std::lock_guard<std::mutex> readLock{read_mutex_};

    bool const revoked = m.revoked();

    if (revoked)
    {
        
        if (auto stream = j_.warn())
            logMftAct(stream, "Revoked", m.masterKey, m.sequence);
    }

    if (iter == map_.end ())
    {
        
        if (auto stream = j_.info())
            logMftAct(stream, "AcceptedNew", m.masterKey, m.sequence);

        if (! revoked)
            signingToMasterKeys_[m.signingKey] = m.masterKey;

        auto masterKey = m.masterKey;
        map_.emplace(std::move(masterKey), std::move(m));
    }
    else
    {
        
        if (auto stream = j_.info())
            logMftAct(stream, "AcceptedUpdate",
                      m.masterKey, m.sequence, iter->second.sequence);

        signingToMasterKeys_.erase (iter->second.signingKey);

        if (! revoked)
            signingToMasterKeys_[m.signingKey] = m.masterKey;

        iter->second = std::move (m);
    }

    return ManifestDisposition::accepted;
}

void
ManifestCache::load (
    DatabaseCon& dbCon, std::string const& dbTable)
{
    std::string const sql =
        "SELECT RawData FROM " + dbTable + ";";
    auto db = dbCon.checkoutDb ();
    soci::blob sociRawData (*db);
    soci::statement st =
        (db->prepare << sql,
             soci::into (sociRawData));
    st.execute ();
    while (st.fetch ())
    {
        std::string serialized;
        convert (sociRawData, serialized);
        if (auto mo = deserializeManifest(serialized))
        {
            if (!mo->verify())
            {
                JLOG(j_.warn())
                    << "Unverifiable manifest in db";
                continue;
            }

            applyManifest (std::move(*mo));
        }
        else
        {
            JLOG(j_.warn())
                << "Malformed manifest in database";
        }
    }
}

bool
ManifestCache::load (
    DatabaseCon& dbCon, std::string const& dbTable,
    std::string const& configManifest,
    std::vector<std::string> const& configRevocation)
{
    load (dbCon, dbTable);

    if (! configManifest.empty())
    {
        auto mo = deserializeManifest(base64_decode(configManifest));
        if (! mo)
        {
            JLOG (j_.error()) << "Malformed validator_token in config";
            return false;
        }

        if (mo->revoked())
        {
            JLOG (j_.warn()) <<
                "Configured manifest revokes public key";
        }

        if (applyManifest (std::move(*mo)) ==
            ManifestDisposition::invalid)
        {
            JLOG (j_.error()) << "Manifest in config was rejected";
            return false;
        }
    }

    if (! configRevocation.empty())
    {
        std::string revocationStr;
        revocationStr.reserve (
            std::accumulate (configRevocation.cbegin(), configRevocation.cend(), std::size_t(0),
                [] (std::size_t init, std::string const& s)
                {
                    return init + s.size();
                }));

        for (auto const& line : configRevocation)
            revocationStr += beast::rfc2616::trim(line);

        auto mo = deserializeManifest(base64_decode(revocationStr));

        if (! mo || ! mo->revoked() ||
            applyManifest (std::move(*mo)) == ManifestDisposition::invalid)
        {
            JLOG (j_.error()) << "Invalid validator key revocation in config";
            return false;
        }
    }

    return true;
}

void ManifestCache::save (
    DatabaseCon& dbCon, std::string const& dbTable,
    std::function <bool (PublicKey const&)> isTrusted)
{
    std::lock_guard<std::mutex> lock{apply_mutex_};

    auto db = dbCon.checkoutDb ();

    soci::transaction tr(*db);
    *db << "DELETE FROM " << dbTable;
    std::string const sql =
        "INSERT INTO " + dbTable + " (RawData) VALUES (:rawData);";
    for (auto const& v : map_)
    {
        if (! v.second.revoked() && ! isTrusted (v.second.masterKey))
        {

            JLOG(j_.info())
               << "Untrusted manifest in cache not saved to db";
            continue;
        }

        soci::blob rawData(*db);
        convert (v.second.serialized, rawData);
        *db << sql,
            soci::use (rawData);
    }
    tr.commit ();
}
}
























