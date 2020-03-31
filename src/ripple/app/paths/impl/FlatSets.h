

#ifndef RIPPLE_APP_PATHS_IMPL_FLAT_SETS_H_INCLUDED
#define RIPPLE_APP_PATHS_IMPL_FLAT_SETS_H_INCLUDED

#include <boost/container/flat_set.hpp>

namespace ripple {


template <class T>
void
SetUnion(
    boost::container::flat_set<T>& dst,
    boost::container::flat_set<T> const& src)
{
    if (src.empty())
        return;

    dst.reserve(dst.size() + src.size());
    dst.insert(
        boost::container::ordered_unique_range_t{}, src.begin(), src.end());
}

}  

#endif








