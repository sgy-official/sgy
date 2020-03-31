

#ifndef RIPPLE_BASICS_ARCHIVE_H_INCLUDED
#define RIPPLE_BASICS_ARCHIVE_H_INCLUDED

#include <boost/filesystem.hpp>

namespace ripple {


void
extractTarLz4(
    boost::filesystem::path const& src,
    boost::filesystem::path const& dst);

} 

#endif








