

#ifndef RIPPLE_RPC_SHARDARCHIVEHANDLER_H_INCLUDED
#define RIPPLE_RPC_SHARDARCHIVEHANDLER_H_INCLUDED

#include <ripple/app/main/Application.h>
#include <ripple/basics/BasicConfig.h>
#include <ripple/basics/StringUtilities.h>
#include <ripple/net/SSLHTTPDownloader.h>

#include <boost/asio/basic_waitable_timer.hpp>
#include <boost/filesystem.hpp>

namespace ripple {
namespace RPC {


class ShardArchiveHandler
    : public std::enable_shared_from_this <ShardArchiveHandler>
{
public:
    ShardArchiveHandler() = delete;
    ShardArchiveHandler(ShardArchiveHandler const&) = delete;
    ShardArchiveHandler& operator= (ShardArchiveHandler&&) = delete;
    ShardArchiveHandler& operator= (ShardArchiveHandler const&) = delete;

    
    ShardArchiveHandler(Application& app, bool validate);

    ~ShardArchiveHandler();

    
    bool
    add(std::uint32_t shardIndex, parsedURL&& url);

    
    bool
    start();

private:
    bool
    next(std::lock_guard<std::mutex>& l);

    void
    complete(boost::filesystem::path dstPath);

    void
    process(boost::filesystem::path const& dstPath);

    void
    remove(std::lock_guard<std::mutex>&);

    std::mutex mutable m_;
    Application& app_;
    std::shared_ptr<SSLHTTPDownloader> downloader_;
    boost::filesystem::path const downloadDir_;
    bool const validate_;
    boost::asio::basic_waitable_timer<std::chrono::steady_clock> timer_;
    bool process_;
    std::map<std::uint32_t, parsedURL> archives_;
    beast::Journal j_;
};

} 
} 

#endif








