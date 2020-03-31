

#include <ripple/app/main/Application.h>
#include <ripple/basics/Log.h>
#include <ripple/basics/StringUtilities.h>
#include <ripple/core/Config.h>
#include <ripple/core/TimeKeeper.h>
#include <ripple/overlay/Cluster.h>
#include <ripple/overlay/ClusterNode.h>
#include <ripple/protocol/jss.h>
#include <ripple/protocol/tokens.h>
#include <boost/regex.hpp>
#include <memory.h>

namespace ripple {

Cluster::Cluster (beast::Journal j)
    : j_ (j)
{
}

boost::optional<std::string>
Cluster::member (PublicKey const& identity) const
{
    std::lock_guard<std::mutex> lock(mutex_);

    auto iter = nodes_.find (identity);
    if (iter == nodes_.end ())
        return boost::none;
    return iter->name ();
}

std::size_t
Cluster::size() const
{
    std::lock_guard<std::mutex> lock(mutex_);

    return nodes_.size();
}

bool
Cluster::update (
    PublicKey const& identity,
    std::string name,
    std::uint32_t loadFee,
    NetClock::time_point reportTime)
{
    std::lock_guard<std::mutex> lock(mutex_);

    std::set<ClusterNode, Comparator>::iterator iter =
        nodes_.find (identity);

    if (iter != nodes_.end ())
    {
        if (reportTime <= iter->getReportTime())
            return false;

        if (name.empty())
            name = iter->name();

        iter = nodes_.erase (iter);
    }

    nodes_.emplace_hint (iter, identity, name, loadFee, reportTime);
    return true;
}

void
Cluster::for_each (
    std::function<void(ClusterNode const&)> func) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto const& ni : nodes_)
        func (ni);
}

bool
Cluster::load (Section const& nodes)
{
    static boost::regex const re (
        "[[:space:]]*"            
        "([[:alnum:]]+)"          
        "(?:"                     
        "[[:space:]]+"            
        "(?:"                     
        "(.*[^[:space:]]+)"       
        "[[:space:]]*"            
        ")?"                      
        ")?"                      
    );

    for (auto const& n : nodes.values())
    {
        boost::smatch match;

        if (!boost::regex_match (n, match, re))
        {
            JLOG (j_.error()) <<
                "Malformed entry: '" << n << "'";
            return false;
        }

        auto const id = parseBase58<PublicKey>(
            TokenType::NodePublic, match[1]);

        if (!id)
        {
            JLOG (j_.error()) <<
                "Invalid node identity: " << match[1];
            return false;
        }

        if (member (*id))
        {
            JLOG (j_.warn()) <<
                "Duplicate node identity: " << match[1];
            continue;
        }

        update(*id, trim_whitespace(match[2]));
    }

    return true;
}

} 
























