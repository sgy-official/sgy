

#ifndef RIPPLE_BASICS_FILEUTILITIES_H_INCLUDED
#define RIPPLE_BASICS_FILEUTILITIES_H_INCLUDED

#include <boost/system/error_code.hpp>
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>

namespace ripple
{


std::string getFileContents(boost::system::error_code& ec,
    boost::filesystem::path const& sourcePath,
    boost::optional<std::size_t> maxSize = boost::none);

}

#endif








