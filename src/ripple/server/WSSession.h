

#ifndef RIPPLE_SERVER_WSSESSION_H_INCLUDED
#define RIPPLE_SERVER_WSSESSION_H_INCLUDED

#include <ripple/server/Handoff.h>
#include <ripple/server/Port.h>
#include <ripple/server/Writer.h>
#include <boost/beast/core/buffers_prefix.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/logic/tribool.hpp>
#include <algorithm>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace ripple {

class WSMsg
{
public:
    WSMsg() = default;
    WSMsg(WSMsg const&) = delete;
    WSMsg& operator=(WSMsg const&) = delete;
    virtual ~WSMsg() = default;

    
    virtual
    std::pair<boost::tribool,
        std::vector<boost::asio::const_buffer>>
    prepare(std::size_t bytes,
        std::function<void(void)> resume) = 0;
};

template<class Streambuf>
class StreambufWSMsg : public WSMsg
{
    Streambuf sb_;
    std::size_t n_ = 0;

public:
    StreambufWSMsg(Streambuf&& sb)
        : sb_(std::move(sb))
    {
    }

    std::pair<boost::tribool,
        std::vector<boost::asio::const_buffer>>
    prepare(std::size_t bytes,
        std::function<void(void)>) override
    {
        if (sb_.size() == 0)
            return{true, {}};
        sb_.consume(n_);
        boost::tribool done;
        if (bytes < sb_.size())
        {
            n_ = bytes;
            done = false;
        }
        else
        {
            n_ = sb_.size();
            done = true;
        }
        auto const pb = boost::beast::buffers_prefix(n_, sb_.data());
        std::vector<boost::asio::const_buffer> vb (
            std::distance(pb.begin(), pb.end()));
        std::copy(pb.begin(), pb.end(), std::back_inserter(vb));
        return{done, vb};
    }
};

struct WSSession
{
    std::shared_ptr<void> appDefined;

    virtual ~WSSession () = default;
    WSSession() = default;
    WSSession(WSSession const&) = delete;
    WSSession& operator=(WSSession const&) = delete;

    virtual
    void
    run() = 0;

    virtual
    Port const&
    port() const = 0;

    virtual
    http_request_type const&
    request() const = 0;

    virtual
    boost::asio::ip::tcp::endpoint const&
    remote_endpoint() const = 0;

    
    virtual
    void
    send(std::shared_ptr<WSMsg> w) = 0;

    virtual void
    close() = 0;

    virtual
    void
    close(boost::beast::websocket::close_reason const& reason) = 0;

    
    virtual
    void
    complete() = 0;
};

} 

#endif








