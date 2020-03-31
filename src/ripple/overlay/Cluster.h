

#ifndef RIPPLE_OVERLAY_CLUSTER_H_INCLUDED
#define RIPPLE_OVERLAY_CLUSTER_H_INCLUDED

#include <ripple/app/main/Application.h>
#include <ripple/basics/chrono.h>
#include <ripple/basics/BasicConfig.h>
#include <ripple/overlay/ClusterNode.h>
#include <ripple/protocol/PublicKey.h>
#include <ripple/beast/hash/uhash.h>
#include <ripple/beast/utility/Journal.h>
#include <functional>
#include <memory>
#include <mutex>
#include <set>
#include <type_traits>

namespace ripple {

class Cluster
{
private:
    struct Comparator
    {
        explicit Comparator() = default;

        using is_transparent = std::true_type;

        bool
        operator() (
            ClusterNode const& lhs,
            ClusterNode const& rhs) const
        {
            return lhs.identity() < rhs.identity();
        }

        bool
        operator() (
            ClusterNode const& lhs,
            PublicKey const& rhs) const
        {
            return lhs.identity() < rhs;
        }

        bool
        operator() (
            PublicKey const& lhs,
            ClusterNode const& rhs) const
        {
            return lhs < rhs.identity();
        }
    };

    std::set<ClusterNode, Comparator> nodes_;
    std::mutex mutable mutex_;
    beast::Journal mutable j_;

public:
    Cluster (beast::Journal j);

    
    boost::optional<std::string>
    member (PublicKey const& node) const;

    
    std::size_t
    size() const;

    
    bool
    update (
        PublicKey const& identity,
        std::string name,
        std::uint32_t loadFee = 0,
        NetClock::time_point reportTime = NetClock::time_point{});

    
    void
    for_each (
        std::function<void(ClusterNode const&)> func) const;

    
    bool
    load (Section const& nodes);
};

} 

#endif








