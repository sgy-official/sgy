

#ifndef RIPPLE_APP_MISC_VALIDATORSITE_H_INCLUDED
#define RIPPLE_APP_MISC_VALIDATORSITE_H_INCLUDED

#include <ripple/app/misc/ValidatorList.h>
#include <ripple/app/misc/detail/Work.h>
#include <ripple/basics/Log.h>
#include <ripple/basics/StringUtilities.h>
#include <ripple/json/json_value.h>
#include <boost/asio.hpp>
#include <mutex>
#include <memory>

namespace ripple {


class ValidatorSite
{
    friend class Work;

private:
    using error_code = boost::system::error_code;
    using clock_type = std::chrono::system_clock;

    struct Site
    {
        struct Status
        {
            clock_type::time_point refreshed;
            ListDisposition disposition;
            std::string message;
        };

        struct Resource
        {
            explicit Resource(std::string uri_);
            const std::string uri;
            parsedURL pUrl;
        };

        explicit Site(std::string uri);

        std::shared_ptr<Resource> loadedResource;

        std::shared_ptr<Resource> startingResource;

        std::shared_ptr<Resource> activeResource;

        unsigned short redirCount;
        std::chrono::minutes refreshInterval;
        clock_type::time_point nextRefresh;
        boost::optional<Status> lastRefreshStatus;
    };

    boost::asio::io_service& ios_;
    ValidatorList& validators_;
    beast::Journal j_;
    std::mutex mutable sites_mutex_;
    std::mutex mutable state_mutex_;

    std::condition_variable cv_;
    std::weak_ptr<detail::Work> work_;
    boost::asio::basic_waitable_timer<clock_type> timer_;

    std::atomic<bool> fetching_;

    std::atomic<bool> pending_;
    std::atomic<bool> stopping_;

    std::vector<Site> sites_;

    const std::chrono::seconds requestTimeout_;

public:
    ValidatorSite (
        boost::asio::io_service& ios,
        ValidatorList& validators,
        beast::Journal j,
        std::chrono::seconds timeout = std::chrono::seconds{20});
    ~ValidatorSite ();

    
    bool
    load (
        std::vector<std::string> const& siteURIs);

    
    void
    start ();

    
    void
    join ();

    
    void
    stop ();

    
    Json::Value
    getJson() const;

private:
    void
    setTimer (std::lock_guard<std::mutex>&);

    void
    onRequestTimeout (
        std::size_t siteIdx,
        error_code const& ec);

    void
    onTimer (
        std::size_t siteIdx,
        error_code const& ec);

    void
    onSiteFetch (
        boost::system::error_code const& ec,
        detail::response_type&& res,
        std::size_t siteIdx);

    void
    onTextFetch(
        boost::system::error_code const& ec,
        std::string const& res,
        std::size_t siteIdx);

    void
    makeRequest (
        std::shared_ptr<Site::Resource> resource,
        std::size_t siteIdx,
        std::lock_guard<std::mutex>& lock);

    void
    parseJsonResponse (
        std::string const& res,
        std::size_t siteIdx,
        std::lock_guard<std::mutex>& lock);

    std::shared_ptr<Site::Resource>
    processRedirect (
        detail::response_type& res,
        std::size_t siteIdx,
        std::lock_guard<std::mutex>& lock);
};

} 

#endif








