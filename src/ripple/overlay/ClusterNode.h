

#ifndef RIPPLE_APP_PEERS_CLUSTERNODESTATUS_H_INCLUDED
#define RIPPLE_APP_PEERS_CLUSTERNODESTATUS_H_INCLUDED

#include <ripple/basics/chrono.h>
#include <ripple/protocol/PublicKey.h>
#include <cstdint>
#include <string>

namespace ripple {

class ClusterNode
{
public:
    ClusterNode() = delete;

    ClusterNode(
            PublicKey const& identity,
            std::string const& name,
            std::uint32_t fee = 0,
            NetClock::time_point rtime = NetClock::time_point{})
        : identity_ (identity)
        , name_(name)
        , mLoadFee(fee)
        , mReportTime(rtime)
    { }

    std::string const& name() const
    {
        return name_;
    }

    std::uint32_t getLoadFee() const
    {
        return mLoadFee;
    }

    NetClock::time_point getReportTime() const
    {
        return mReportTime;
    }

    PublicKey const&
    identity () const
    {
        return identity_;
    }

private:
    PublicKey const identity_;
    std::string name_;
    std::uint32_t mLoadFee = 0;
    NetClock::time_point mReportTime = {};
};

} 

#endif








