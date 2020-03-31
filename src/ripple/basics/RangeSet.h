

#ifndef RIPPLE_BASICS_RANGESET_H_INCLUDED
#define RIPPLE_BASICS_RANGESET_H_INCLUDED

#include <string>
#include <boost/optional.hpp>
#include <boost/icl/closed_interval.hpp>
#include <boost/icl/interval_set.hpp>
#include <boost/serialization/split_free.hpp>

namespace ripple
{


template <class T>
using ClosedInterval = boost::icl::closed_interval<T>;


template <class T>
ClosedInterval<T>
range(T low, T high)
{
    return ClosedInterval<T>(low, high);
}


template <class T>
using RangeSet = boost::icl::interval_set<T, std::less, ClosedInterval<T>>;



template <class T>
std::string to_string(ClosedInterval<T> const & ci)
{
    if (ci.first() == ci.last())
        return std::to_string(ci.first());
    return std::to_string(ci.first()) + "-" + std::to_string(ci.last());
}


template <class T>
std::string to_string(RangeSet<T> const & rs)
{
    using ripple::to_string;

    if (rs.empty())
        return "empty";
    std::string res = "";
    for (auto const & interval : rs)
    {
        if (!res.empty())
            res += ",";
        res += to_string(interval);
    }
    return res;
}


template <class T>
boost::optional<T>
prevMissing(RangeSet<T> const & rs, T t, T minVal = 0)
{
    if (rs.empty() || t == minVal)
        return boost::none;
    RangeSet<T> tgt{ ClosedInterval<T>{minVal, t - 1} };
    tgt -= rs;
    if (tgt.empty())
        return boost::none;
    return boost::icl::last(tgt);
}
}  



namespace boost {
namespace serialization {
template <class Archive, class T>
void
save(Archive& ar,
    ripple::ClosedInterval<T> const& ci,
    const unsigned int version)
{
    auto l = ci.lower();
    auto u = ci.upper();
    ar << l << u;
}

template <class Archive, class T>
void
load(Archive& ar, ripple::ClosedInterval<T>& ci, const unsigned int version)
{
    T low, up;
    ar >> low >> up;
    ci = ripple::ClosedInterval<T>{low, up};
}

template <class Archive, class T>
void
serialize(Archive& ar,
    ripple::ClosedInterval<T>& ci,
    const unsigned int version)
{
    split_free(ar, ci, version);
}

template <class Archive, class T>
void
save(Archive& ar, ripple::RangeSet<T> const& rs, const unsigned int version)
{
    auto s = rs.iterative_size();
    ar << s;
    for (auto const& r : rs)
        ar << r;
}

template <class Archive, class T>
void
load(Archive& ar, ripple::RangeSet<T>& rs, const unsigned int version)
{
    rs.clear();
    std::size_t intervals;
    ar >> intervals;
    for (std::size_t i = 0; i < intervals; ++i)
    {
        ripple::ClosedInterval<T> ci;
        ar >> ci;
        rs.insert(ci);
    }
}

template <class Archive, class T>
void
serialize(Archive& ar, ripple::RangeSet<T>& rs, const unsigned int version)
{
    split_free(ar, rs, version);
}

}  
}  
#endif








