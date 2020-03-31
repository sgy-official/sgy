

#ifndef RIPPLE_APP_MISC_CANONICALTXSET_H_INCLUDED
#define RIPPLE_APP_MISC_CANONICALTXSET_H_INCLUDED

#include <ripple/protocol/RippleLedgerHash.h>
#include <ripple/protocol/STTx.h>

namespace ripple {


class CanonicalTXSet
{
private:
    class Key
    {
    public:
        Key (uint256 const& account, std::uint32_t seq, uint256 const& id)
            : mAccount (account)
            , mTXid (id)
            , mSeq (seq)
        {
        }

        bool operator<  (Key const& rhs) const;
        bool operator>  (Key const& rhs) const;
        bool operator<= (Key const& rhs) const;
        bool operator>= (Key const& rhs) const;

        bool operator== (Key const& rhs) const
        {
            return mTXid == rhs.mTXid;
        }
        bool operator!= (Key const& rhs) const
        {
            return mTXid != rhs.mTXid;
        }

        uint256 const& getTXID () const
        {
            return mTXid;
        }

    private:
        uint256 mAccount;
        uint256 mTXid;
        std::uint32_t mSeq;
    };

    uint256 accountKey (AccountID const& account);

public:
    using const_iterator = std::map <Key, std::shared_ptr<STTx const>>::const_iterator;

public:
    explicit CanonicalTXSet (LedgerHash const& saltHash)
        : salt_ (saltHash)
    {
    }

    void insert (std::shared_ptr<STTx const> const& txn);

    std::vector<std::shared_ptr<STTx const>>
    prune(AccountID const& account, std::uint32_t const seq);

    void reset (LedgerHash const& salt)
    {
        salt_ = salt;
        map_.clear ();
    }

    const_iterator erase (const_iterator const& it)
    {
        return map_.erase(it);
    }

    const_iterator begin () const
    {
        return map_.begin();
    }

    const_iterator end() const
    {
        return map_.end();
    }

    size_t size () const
    {
        return map_.size ();
    }
    bool empty () const
    {
        return map_.empty ();
    }

    uint256 const& key() const
    {
        return salt_;
    }

private:
    std::map <Key, std::shared_ptr<STTx const>> map_;

    uint256 salt_;
};

} 

#endif








