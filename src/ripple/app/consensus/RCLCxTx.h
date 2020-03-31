

#ifndef RIPPLE_APP_CONSENSUS_RCLCXTX_H_INCLUDED
#define RIPPLE_APP_CONSENSUS_RCLCXTX_H_INCLUDED

#include <ripple/app/misc/CanonicalTXSet.h>
#include <ripple/basics/chrono.h>
#include <ripple/protocol/UintTypes.h>
#include <ripple/shamap/SHAMap.h>

namespace ripple {


class RCLCxTx
{
public:
    using ID = uint256;

    
    RCLCxTx(SHAMapItem const& txn) : tx_{txn}
    {
    }

    ID const&
    id() const
    {
        return tx_.key();
    }

    SHAMapItem const tx_;
};


class RCLTxSet
{
public:
    using ID = uint256;
    using Tx = RCLCxTx;

    class MutableTxSet
    {
        friend class RCLTxSet;
        std::shared_ptr<SHAMap> map_;

    public:
        MutableTxSet(RCLTxSet const& src) : map_{src.map_->snapShot(true)}
        {
        }

        
        bool
        insert(Tx const& t)
        {
            return map_->addItem(
                SHAMapItem{t.id(), t.tx_.peekData()}, true, false);
        }

        
        bool
        erase(Tx::ID const& entry)
        {
            return map_->delItem(entry);
        }
    };

    
    RCLTxSet(std::shared_ptr<SHAMap> m) : map_{std::move(m)}
    {
        assert(map_);
    }

    
    RCLTxSet(MutableTxSet const& m) : map_{m.map_->snapShot(false)}
    {
    }

    
    bool
    exists(Tx::ID const& entry) const
    {
        return map_->hasItem(entry);
    }

    
    std::shared_ptr<const SHAMapItem> const&
    find(Tx::ID const& entry) const
    {
        return map_->peekItem(entry);
    }

    ID
    id() const
    {
        return map_->getHash().as_uint256();
    }

    
    std::map<Tx::ID, bool>
    compare(RCLTxSet const& j) const
    {
        SHAMap::Delta delta;

        map_->compare(*(j.map_), delta, 65536);

        std::map<uint256, bool> ret;
        for (auto const& item : delta)
        {
            assert(
                (item.second.first && !item.second.second) ||
                (item.second.second && !item.second.first));

            ret[item.first] = static_cast<bool>(item.second.first);
        }
        return ret;
    }

    std::shared_ptr<SHAMap> map_;
};
}
#endif








