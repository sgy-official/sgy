

#ifndef RIPPLE_CORE_LOADFEETRACK_H_INCLUDED
#define RIPPLE_CORE_LOADFEETRACK_H_INCLUDED

#include <ripple/json/json_value.h>
#include <ripple/beast/utility/Journal.h>
#include <algorithm>
#include <cstdint>
#include <mutex>

namespace ripple {

struct Fees;


class LoadFeeTrack final
{
public:
    explicit LoadFeeTrack (beast::Journal journal =
        beast::Journal(beast::Journal::getNullSink()))
        : j_ (journal)
        , localTxnLoadFee_ (lftNormalFee)
        , remoteTxnLoadFee_ (lftNormalFee)
        , clusterTxnLoadFee_ (lftNormalFee)
        , raiseCount_ (0)
    {
    }

    ~LoadFeeTrack() = default;

    void setRemoteFee (std::uint32_t f)
    {
        std::lock_guard <std::mutex> sl (lock_);
        remoteTxnLoadFee_ = f;
    }

    std::uint32_t getRemoteFee () const
    {
        std::lock_guard <std::mutex> sl (lock_);
        return remoteTxnLoadFee_;
    }

    std::uint32_t getLocalFee () const
    {
        std::lock_guard <std::mutex> sl (lock_);
        return localTxnLoadFee_;
    }

    std::uint32_t getClusterFee () const
    {
        std::lock_guard <std::mutex> sl (lock_);
        return clusterTxnLoadFee_;
    }

    std::uint32_t getLoadBase () const
    {
        return lftNormalFee;
    }

    std::uint32_t getLoadFactor () const
    {
        std::lock_guard <std::mutex> sl (lock_);
        return std::max({ clusterTxnLoadFee_, localTxnLoadFee_, remoteTxnLoadFee_ });
    }

    std::pair<std::uint32_t, std::uint32_t>
    getScalingFactors() const
    {
        std::lock_guard<std::mutex> sl(lock_);

        return std::make_pair(
            std::max(localTxnLoadFee_, remoteTxnLoadFee_),
            std::max(remoteTxnLoadFee_, clusterTxnLoadFee_));
    }


    void setClusterFee (std::uint32_t fee)
    {
        std::lock_guard <std::mutex> sl (lock_);
        clusterTxnLoadFee_ = fee;
    }

    bool raiseLocalFee ();
    bool lowerLocalFee ();

    bool isLoadedLocal () const
    {
        std::lock_guard <std::mutex> sl (lock_);
        return (raiseCount_ != 0) || (localTxnLoadFee_ != lftNormalFee);
    }

    bool isLoadedCluster () const
    {
        std::lock_guard <std::mutex> sl (lock_);
        return (raiseCount_ != 0) || (localTxnLoadFee_ != lftNormalFee) ||
            (clusterTxnLoadFee_ != lftNormalFee);
    }

private:
    static std::uint32_t constexpr lftNormalFee = 256;        
    static std::uint32_t constexpr lftFeeIncFraction = 4;     
    static std::uint32_t constexpr lftFeeDecFraction = 4;     
    static std::uint32_t constexpr lftFeeMax = lftNormalFee * 1000000;

    beast::Journal j_;
    std::mutex mutable lock_;

    std::uint32_t localTxnLoadFee_;        
    std::uint32_t remoteTxnLoadFee_;       
    std::uint32_t clusterTxnLoadFee_;      
    std::uint32_t raiseCount_;
};


std::uint64_t scaleFeeLoad(std::uint64_t fee, LoadFeeTrack const& feeTrack,
    Fees const& fees, bool bUnlimited);

} 

#endif








