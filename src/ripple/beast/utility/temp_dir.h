

#ifndef BEAST_UTILITY_TEMP_DIR_H_INCLUDED
#define BEAST_UTILITY_TEMP_DIR_H_INCLUDED

#include <boost/filesystem.hpp>
#include <string>

namespace beast {


class temp_dir
{
    boost::filesystem::path path_;

public:
#if ! GENERATING_DOCS
    temp_dir(const temp_dir&) = delete;
    temp_dir& operator=(const temp_dir&) = delete;
#endif

    temp_dir()
    {
        auto const dir =
            boost::filesystem::temp_directory_path();
        do
        {
            path_ =
                dir / boost::filesystem::unique_path();
        }
        while(boost::filesystem::exists(path_));
        boost::filesystem::create_directory (path_);
    }

    ~temp_dir()
    {
        boost::system::error_code ec;
        boost::filesystem::remove_all(path_, ec);
    }

    std::string
    path() const
    {
        return path_.string();
    }

    
    std::string
    file(std::string const& name) const
    {
        return (path_ / name).string();
    }
};

} 

#endif








