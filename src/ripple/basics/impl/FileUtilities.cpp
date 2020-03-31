

#include <ripple/basics/FileUtilities.h>

namespace ripple
{

std::string getFileContents(boost::system::error_code& ec,
    boost::filesystem::path const& sourcePath,
    boost::optional<std::size_t> maxSize)
{
    using namespace boost::filesystem;
    using namespace boost::system::errc;

    path fullPath{ canonical(sourcePath, ec) };
    if (ec)
        return {};

    if (maxSize && (file_size(fullPath, ec) > *maxSize || ec))
    {
        if (!ec)
            ec = make_error_code(file_too_large);
        return {};
    }

    ifstream fileStream(fullPath, std::ios::in);

    if (!fileStream)
    {
        ec = make_error_code(static_cast<errc_t>(errno));
        return {};
    }

    const std::string result{ std::istreambuf_iterator<char>{fileStream},
        std::istreambuf_iterator<char>{} };

    if (fileStream.bad ())
    {
        ec = make_error_code(static_cast<errc_t>(errno));
        return {};
    }

    return result;
}

}
























