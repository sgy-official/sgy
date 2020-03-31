

#ifndef RIPPLE_APP_MISC_VALIDATORLIST_H_INCLUDED
#define RIPPLE_APP_MISC_VALIDATORLIST_H_INCLUDED

#include <ripple/app/misc/Manifest.h>
#include <ripple/basics/Log.h>
#include <ripple/basics/UnorderedContainers.h>
#include <ripple/core/TimeKeeper.h>
#include <ripple/crypto/csprng.h>
#include <ripple/json/json_value.h>
#include <ripple/protocol/PublicKey.h>
#include <boost/iterator/counting_iterator.hpp>
#include <boost/range/adaptors.hpp>
#include <mutex>
#include <shared_mutex>
#include <numeric>

namespace ripple {

enum class ListDisposition
{
    accepted = 0,

    same_sequence,

    unsupported_version,

    untrusted,

    stale,

    invalid
};

std::string
to_string(ListDisposition disposition);


struct TrustChanges
{
    explicit TrustChanges() = default;

    hash_set<NodeID> added;
    hash_set<NodeID> removed;
};


class ValidatorList
{
    struct PublisherList
    {
        explicit PublisherList() = default;

        bool available;
        std::vector<PublicKey> list;
        std::size_t sequence;
        TimeKeeper::time_point expiration;
        std::string siteUri;
    };

    ManifestCache& validatorManifests_;
    ManifestCache& publisherManifests_;
    TimeKeeper& timeKeeper_;
    beast::Journal j_;
    std::shared_timed_mutex mutable mutex_;

    std::atomic<std::size_t> quorum_;
    boost::optional<std::size_t> minimumQuorum_;

    hash_map<PublicKey, PublisherList> publisherLists_;

    hash_map<PublicKey, std::size_t> keyListings_;

    hash_set<PublicKey> trustedKeys_;

    PublicKey localPubKey_;

    static constexpr std::uint32_t requiredListVersion = 1;

public:
    ValidatorList (
        ManifestCache& validatorManifests,
        ManifestCache& publisherManifests,
        TimeKeeper& timeKeeper,
        beast::Journal j,
        boost::optional<std::size_t> minimumQuorum = boost::none);
    ~ValidatorList () = default;

    
    bool
    load (
        PublicKey const& localSigningKey,
        std::vector<std::string> const& configKeys,
        std::vector<std::string> const& publisherKeys);

    
    ListDisposition
    applyList (
        std::string const& manifest,
        std::string const& blob,
        std::string const& signature,
        std::uint32_t version,
        std::string siteUri);

    
    TrustChanges
    updateTrusted (hash_set<NodeID> const& seenValidators);

    
    std::size_t
    quorum () const
    {
        return quorum_;
    }

    
    bool
    trusted (
        PublicKey const& identity) const;

    
    bool
    listed (
        PublicKey const& identity) const;

    
    boost::optional<PublicKey>
    getTrustedKey (
        PublicKey const& identity) const;

    
    boost::optional<PublicKey>
    getListedKey (
        PublicKey const& identity) const;

    
    bool
    trustedPublisher (
        PublicKey const& identity) const;

    
    PublicKey
    localPublicKey () const;

    
    void
    for_each_listed (
        std::function<void(PublicKey const&, bool)> func) const;

    
    std::size_t
    count() const;

    
    boost::optional<TimeKeeper::time_point>
    expires() const;

    
    Json::Value
    getJson() const;

    using QuorumKeys = std::pair<std::size_t const, hash_set<PublicKey>>;
    
    QuorumKeys
    getQuorumKeys() const
    {
        std::shared_lock<std::shared_timed_mutex> read_lock{mutex_};
        return {quorum_, trustedKeys_};
    }


private:
    
    ListDisposition
    verify (
        Json::Value& list,
        PublicKey& pubKey,
        std::string const& manifest,
        std::string const& blob,
        std::string const& signature);

    
    bool
    removePublisherList (PublicKey const& publisherKey);

    
    std::size_t
    calculateQuorum (
        std::size_t trusted, std::size_t seen);
};
} 

#endif








