

#ifndef RIPPLE_APP_CONSENSUS_RCLCENSORSHIPDETECTOR_H_INCLUDED
#define RIPPLE_APP_CONSENSUS_RCLCENSORSHIPDETECTOR_H_INCLUDED

#include <ripple/basics/algorithm.h>
#include <ripple/shamap/SHAMap.h>
#include <algorithm>
#include <utility>
#include <vector>

namespace ripple {

template <class TxID, class Sequence>
class RCLCensorshipDetector
{
public:
    struct TxIDSeq
    {
        TxID txid;
        Sequence seq;

        TxIDSeq (TxID const& txid_, Sequence const& seq_)
        : txid (txid_)
        , seq (seq_)
        { }
    };

    friend bool operator< (TxIDSeq const& lhs, TxIDSeq const& rhs)
    {
        if (lhs.txid != rhs.txid)
            return lhs.txid < rhs.txid;
        return lhs.seq < rhs.seq;
    }

    friend bool operator< (TxIDSeq const& lhs, TxID const& rhs)
    {
        return lhs.txid < rhs;
    }

    friend bool operator< (TxID const& lhs, TxIDSeq const& rhs)
    {
        return lhs < rhs.txid;
    }

    using TxIDSeqVec = std::vector<TxIDSeq>;

private:
    TxIDSeqVec tracker_;

public:
    RCLCensorshipDetector() = default;

    
    void propose(TxIDSeqVec proposed)
    {
        std::sort(proposed.begin(), proposed.end());
        generalized_set_intersection(proposed.begin(), proposed.end(),
            tracker_.cbegin(), tracker_.cend(),
            [](auto& x, auto const& y) {x.seq = y.seq;},
            [](auto const& x, auto const& y) {return x.txid < y.txid;});
        tracker_ = std::move(proposed);
    }

    
    template <class Predicate>
    void check(
        std::vector<TxID> accepted,
        Predicate&& pred)
    {
        auto acceptTxid = accepted.begin();
        auto const ae = accepted.end();
        std::sort(acceptTxid, ae);


        auto i = remove_if_intersect_or_match(tracker_.begin(), tracker_.end(),
                     accepted.begin(), accepted.end(),
                     [&pred](auto const& x) {return pred(x.txid, x.seq);},
                     std::less<void>{});
        tracker_.erase(i, tracker_.end());
    }

    
    void reset()
    {
        tracker_.clear();
    }
};

}

#endif








