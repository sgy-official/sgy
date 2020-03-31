

#ifndef RIPPLE_APP_MISC_DETAIL_WORKFILE_H_INCLUDED
#define RIPPLE_APP_MISC_DETAIL_WORKFILE_H_INCLUDED

#include <ripple/app/misc/detail/Work.h>
#include <ripple/basics/ByteUtilities.h>
#include <ripple/basics/FileUtilities.h>
#include <cassert>
#include <cerrno>

namespace ripple {

namespace detail {

class WorkFile: public Work
    , public std::enable_shared_from_this<WorkFile>
{
protected:
    using error_code = boost::system::error_code;
    using response_type = std::string;

public:
    using callback_type =
        std::function<void(error_code const&, response_type const&)>;
public:
    WorkFile(
        std::string const& path,
        boost::asio::io_service& ios, callback_type cb);
    ~WorkFile();

    void run() override;

    void cancel() override;

private:
    std::string path_;
    callback_type cb_;
    boost::asio::io_service& ios_;
    boost::asio::io_service::strand strand_;

};


WorkFile::WorkFile(
    std::string const& path,
    boost::asio::io_service& ios, callback_type cb)
    : path_(path)
    , cb_(std::move(cb))
    , ios_(ios)
    , strand_(ios)
{
}

WorkFile::~WorkFile()
{
    if (cb_)
        cb_ (make_error_code(boost::system::errc::interrupted), {});
}

void
WorkFile::run()
{
    if (! strand_.running_in_this_thread())
        return ios_.post(strand_.wrap (std::bind(
            &WorkFile::run, shared_from_this())));

    error_code ec;
    auto const fileContents = getFileContents(ec, path_, megabytes(1));

    assert(cb_);
    cb_(ec, fileContents);
    cb_ = nullptr;
}

void
WorkFile::cancel()
{
}


} 

} 

#endif








