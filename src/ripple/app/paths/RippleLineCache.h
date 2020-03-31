

#ifndef RIPPLE_APP_PATHS_RIPPLELINECACHE_H_INCLUDED
#define RIPPLE_APP_PATHS_RIPPLELINECACHE_H_INCLUDED

#include <ripple/app/ledger/Ledger.h>
#include <ripple/app/paths/RippleState.h>
#include <ripple/basics/hardened_hash.h>
#include <cstddef>
#include <memory>
#include <mutex>
#include <vector>

namespace ripple {

class RippleLineCache
{
public:
    explicit
    RippleLineCache (
        std::shared_ptr <ReadView const> const& l);

    std::shared_ptr <ReadView const> const&
    getLedger () const
    {
        return mLedger;
    }

    std::vector<RippleState::pointer> const&
    getRippleLines (AccountID const& accountID);

private:
    std::mutex mLock;

    ripple::hardened_hash<> hasher_;
    std::shared_ptr <ReadView const> mLedger;

    struct AccountKey
    {
        AccountID account_;
        std::size_t hash_value_;

        AccountKey (AccountID const& account, std::size_t hash)
            : account_ (account)
            , hash_value_ (hash)
        { }

        AccountKey (AccountKey const& other) = default;

        AccountKey&
        operator=(AccountKey const& other) = default;

        bool operator== (AccountKey const& lhs) const
        {
            return hash_value_ == lhs.hash_value_ && account_ == lhs.account_;
        }

        std::size_t
        get_hash () const
        {
            return hash_value_;
        }

        struct Hash
        {
            explicit Hash() = default;

            std::size_t
            operator () (AccountKey const& key) const noexcept
            {
                return key.get_hash ();
            }
        };
    };

    hash_map <
        AccountKey,
        std::vector <RippleState::pointer>,
        AccountKey::Hash> lines_;
};

} 

#endif








